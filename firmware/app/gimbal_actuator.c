#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "gimbal_actuator.h"

#if defined(CONFIG_SGL200_ACTUATOR_SERVO_BUS)
#include "feetech_servo.h"
#endif

LOG_MODULE_REGISTER(gimbal_actuator, LOG_LEVEL_INF);

/* Angle limits (deg) */
#define PITCH_MIN_DEG  (-90.0f)
#define PITCH_MAX_DEG  ( 30.0f)
#define YAW_MIN_DEG    (-160.0f)
#define YAW_MAX_DEG    ( 160.0f)

#if defined(CONFIG_SGL200_ACTUATOR_SERVO_PWM)
#define SERVO_PWM_PERIOD_US   20000U
#define SERVO_PWM_MIN_US       1000U
#define SERVO_PWM_NEUTRAL_US   1500U
#define SERVO_PWM_MAX_US       2000U

static const struct pwm_dt_spec pitch_pwm =
	PWM_DT_SPEC_GET(DT_ALIAS(gimbal_pitch_pwm));
static const struct pwm_dt_spec yaw_pwm =
	PWM_DT_SPEC_GET(DT_ALIAS(gimbal_yaw_pwm));

static uint32_t angle_to_pulse_us(float angle_deg, float min_deg, float max_deg)
{
	if (angle_deg < min_deg) {
		angle_deg = min_deg;
	}
	if (angle_deg > max_deg) {
		angle_deg = max_deg;
	}

	float span = max_deg - min_deg;
	if (span <= 0.0f) {
		return SERVO_PWM_NEUTRAL_US;
	}

	float normalized = (angle_deg - min_deg) / span;
	float pulse_us = (float)SERVO_PWM_MIN_US +
			 normalized * (float)(SERVO_PWM_MAX_US - SERVO_PWM_MIN_US);

	return (uint32_t)(pulse_us + 0.5f);
}
#endif

int gimbal_actuator_init(void)
{
#if defined(CONFIG_SGL200_ACTUATOR_SERVO_BUS)
	int ret = feetech_init();

	if (ret) {
		return ret;
	}
	LOG_INF("Gimbal actuator ready: servo bus");
	return 0;
#elif defined(CONFIG_SGL200_ACTUATOR_SERVO_PWM)
	if (!pwm_is_ready_dt(&pitch_pwm)) {
		LOG_ERR("Pitch servo PWM is not ready");
		return -ENODEV;
	}
	if (!pwm_is_ready_dt(&yaw_pwm)) {
		LOG_ERR("Yaw servo PWM is not ready");
		return -ENODEV;
	}

	int ret = pwm_set_dt(&pitch_pwm, PWM_USEC(SERVO_PWM_PERIOD_US),
			     PWM_USEC(SERVO_PWM_NEUTRAL_US));
	if (ret) {
		return ret;
	}

	ret = pwm_set_dt(&yaw_pwm, PWM_USEC(SERVO_PWM_PERIOD_US),
			 PWM_USEC(SERVO_PWM_NEUTRAL_US));
	if (ret) {
		return ret;
	}

	LOG_INF("Gimbal actuator ready: servo PWM");
	return 0;
#elif defined(CONFIG_SGL200_ACTUATOR_BLDC)
	LOG_ERR("Gimbal actuator BLDC backend is not implemented yet");
	return -ENOTSUP;
#else
	LOG_ERR("No gimbal actuator backend selected");
	return -ENODEV;
#endif
}

int gimbal_actuator_set_angle(enum gimbal_axis_t axis, float angle_deg)
{
#if defined(CONFIG_SGL200_ACTUATOR_SERVO_BUS)
	switch (axis) {
	case GIMBAL_AXIS_PITCH: {
		uint16_t pos = feetech_angle_to_pos(angle_deg, PITCH_MIN_DEG, PITCH_MAX_DEG);

		return feetech_set_goal_position(FEETECH_ID_PITCH, pos);
	}
	case GIMBAL_AXIS_YAW: {
		uint16_t pos = feetech_angle_to_pos(angle_deg, YAW_MIN_DEG, YAW_MAX_DEG);

		return feetech_set_goal_position(FEETECH_ID_YAW, pos);
	}
	default:
		return -EINVAL;
	}
#elif defined(CONFIG_SGL200_ACTUATOR_SERVO_PWM)
	switch (axis) {
	case GIMBAL_AXIS_PITCH:
		return pwm_set_dt(&pitch_pwm, PWM_USEC(SERVO_PWM_PERIOD_US),
				  PWM_USEC(angle_to_pulse_us(angle_deg,
							     PITCH_MIN_DEG,
							     PITCH_MAX_DEG)));
	case GIMBAL_AXIS_YAW:
		return pwm_set_dt(&yaw_pwm, PWM_USEC(SERVO_PWM_PERIOD_US),
				  PWM_USEC(angle_to_pulse_us(angle_deg,
							     YAW_MIN_DEG,
							     YAW_MAX_DEG)));
	default:
		return -EINVAL;
	}
#elif defined(CONFIG_SGL200_ACTUATOR_BLDC)
	ARG_UNUSED(axis);
	ARG_UNUSED(angle_deg);
	return -ENOTSUP;
#else
	ARG_UNUSED(axis);
	ARG_UNUSED(angle_deg);
	return -ENODEV;
#endif
}
