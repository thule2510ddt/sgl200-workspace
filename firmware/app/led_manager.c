#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "led_manager.h"

LOG_MODULE_REGISTER(led_manager, LOG_LEVEL_INF);

/* PWM period = 50 µs → 20 kHz */
#define LED_PWM_PERIOD_US  50U

/* Soft-start: 500 ms at 10 ms tick = 50 steps */
#define SOFT_START_STEPS   50U

/* Strobe timings (in 10 ms ticks) */
#define STROBE_WHITE_ON_TICKS   50U  /* 500 ms on  */
#define STROBE_WHITE_OFF_TICKS  50U  /* 500 ms off */
#define STROBE_POLICE_TICKS     25U  /* 250 ms per colour */
#define SOS_DOT_TICKS           20U  /* 200 ms */
#define SOS_DASH_TICKS          60U  /* 600 ms */
#define SOS_GAP_TICKS           20U  /* 200 ms between elements */
#define SOS_LETTER_GAP_TICKS    60U  /* 600 ms between S and O */

/* SOS pattern: S (. . .) O (- - -) S (. . .) then long gap */
static const uint8_t sos_pattern[] = {
	/* S */
	SOS_DOT_TICKS, SOS_GAP_TICKS,
	SOS_DOT_TICKS, SOS_GAP_TICKS,
	SOS_DOT_TICKS, SOS_LETTER_GAP_TICKS,
	/* O */
	SOS_DASH_TICKS, SOS_GAP_TICKS,
	SOS_DASH_TICKS, SOS_GAP_TICKS,
	SOS_DASH_TICKS, SOS_LETTER_GAP_TICKS,
	/* S */
	SOS_DOT_TICKS, SOS_GAP_TICKS,
	SOS_DOT_TICKS, SOS_GAP_TICKS,
	SOS_DOT_TICKS, 100U, /* long gap before repeat */
};

static const struct pwm_dt_spec pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(led_pwm));

static const struct gpio_dt_spec led_red =
	GPIO_DT_SPEC_GET(DT_ALIAS(led_red), gpios);
static const struct gpio_dt_spec led_blue =
	GPIO_DT_SPEC_GET(DT_ALIAS(led_blue), gpios);

static enum led_fsm_state_t fsm_state = LED_FSM_IDLE;
static struct led_command_t last_cmd;
static uint32_t tick_count;
static uint8_t sos_idx;
static uint8_t soft_start_step;

static void set_pwm(uint8_t brightness_pct)
{
	uint32_t duty_us = (uint32_t)LED_PWM_PERIOD_US * brightness_pct / 100U;

	pwm_set_dt(&pwm_led, PWM_USEC(LED_PWM_PERIOD_US), PWM_USEC(duty_us));
}

static void aux_gpio(bool red, bool blue)
{
	gpio_pin_set_dt(&led_red,  red  ? 1 : 0);
	gpio_pin_set_dt(&led_blue, blue ? 1 : 0);
}

static uint8_t throttled_brightness(const struct thermal_state_t *thermal)
{
	float max_c = (thermal->led_ntc_c > thermal->driver_ntc_c)
		      ? thermal->led_ntc_c : thermal->driver_ntc_c;
	float factor = 1.0f - 0.1f * (max_c - 75.0f);

	if (factor < 0.2f) {
		factor = 0.2f;
	}
	float brt = (float)last_cmd.brightness_pct * factor;

	return (uint8_t)brt;
}

int led_manager_init(void)
{
	if (!pwm_is_ready_dt(&pwm_led)) {
		LOG_ERR("PWM LED not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&led_red) || !gpio_is_ready_dt(&led_blue)) {
		LOG_ERR("AUX LED GPIO not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&led_red,  GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
	set_pwm(0U);
	LOG_INF("LED manager ready");
	return 0;
}

void led_manager_tick(const struct led_command_t *cmd,
		      const struct thermal_state_t *thermal,
		      bool fc_armed)
{
	/* Emergency off is terminal */
	if (fsm_state == LED_FSM_EMERGENCY_OFF) {
		set_pwm(0U);
		aux_gpio(false, false);
		return;
	}

	/* Thermal emergency overrides everything */
	if (thermal->emergency_shutdown) {
		fsm_state = LED_FSM_EMERGENCY_OFF;
		set_pwm(0U);
		aux_gpio(false, false);
		LOG_ERR("LED emergency off");
		return;
	}

	/* Apply new command if provided */
	if (cmd != NULL) {
		last_cmd = *cmd;
	}

	/* Thermal throttle check (non-emergency) */
	if (thermal->throttle_active &&
	    fsm_state != LED_FSM_IDLE &&
	    fsm_state != LED_FSM_THERMAL_THROTTLE) {
		fsm_state  = LED_FSM_THERMAL_THROTTLE;
		tick_count = 0U;
	}

	/* FC disarmed → force idle */
	if (!fc_armed && fsm_state != LED_FSM_IDLE) {
		fsm_state = LED_FSM_IDLE;
		set_pwm(0U);
		aux_gpio(false, false);
		return;
	}

	switch (fsm_state) {

	case LED_FSM_IDLE:
		set_pwm(0U);
		aux_gpio(false, false);
		if (fc_armed && last_cmd.mode != LED_MODE_IDLE &&
		    last_cmd.brightness_pct > 0U) {
			fsm_state       = LED_FSM_SOFT_START;
			soft_start_step = 0U;
			tick_count      = 0U;
			LOG_INF("LED soft-start → target %u%%", last_cmd.brightness_pct);
		}
		break;

	case LED_FSM_SOFT_START:
		soft_start_step++;
		{
			uint8_t brt = (uint8_t)((uint32_t)last_cmd.brightness_pct
						* soft_start_step / SOFT_START_STEPS);
			set_pwm(brt);
		}
		if (soft_start_step >= SOFT_START_STEPS) {
			fsm_state  = (enum led_fsm_state_t)((int)LED_FSM_NORMAL + (int)last_cmd.mode - 1);
			tick_count = 0U;
		}
		break;

	case LED_FSM_NORMAL:
		set_pwm(last_cmd.brightness_pct);
		aux_gpio(false, false);
		if (last_cmd.mode != LED_MODE_NORMAL) {
			fsm_state  = (enum led_fsm_state_t)((int)LED_FSM_NORMAL + (int)last_cmd.mode - 1);
			tick_count = 0U;
		}
		break;

	case LED_FSM_STROBE_WHITE:
		tick_count++;
		{
			bool on = ((tick_count / STROBE_WHITE_ON_TICKS) % 2U) == 0U;

			set_pwm(on ? last_cmd.brightness_pct : 0U);
		}
		break;

	case LED_FSM_STROBE_POLICE:
		tick_count++;
		{
			uint8_t phase = (uint8_t)((tick_count / STROBE_POLICE_TICKS) % 2U);

			set_pwm(0U);
			aux_gpio(phase == 0U, phase == 1U);
		}
		break;

	case LED_FSM_STROBE_SOS:
		if (sos_idx >= sizeof(sos_pattern)) {
			sos_idx = 0U;
		}
		if (tick_count == 0U) {
			bool on = (sos_idx % 2U) == 0U;

			set_pwm(on ? last_cmd.brightness_pct : 0U);
		}
		tick_count++;
		if (tick_count >= sos_pattern[sos_idx]) {
			sos_idx++;
			tick_count = 0U;
		}
		break;

	case LED_FSM_THERMAL_THROTTLE:
		if (!thermal->throttle_active) {
			fsm_state = LED_FSM_NORMAL;
		} else {
			set_pwm(throttled_brightness(thermal));
		}
		break;

	case LED_FSM_EMERGENCY_OFF:
		break;
	}
}
