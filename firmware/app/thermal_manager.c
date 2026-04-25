#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "thermal_manager.h"

LOG_MODULE_REGISTER(thermal, LOG_LEVEL_INF);

/* NTC parameters: 10 kΩ at 25 °C, B = 3950 K, series R = 10 kΩ, VDD ref */
#define NTC_R0      10000.0f
#define NTC_B       3950.0f
#define NTC_T0_K    298.15f
#define NTC_RSERIES 10000.0f

#define ADC_MAX_VAL  4095

#define THROTTLE_THRESHOLD_C  75.0f
#define SHUTDOWN_THRESHOLD_C  95.0f

static const struct device *adc_dev;

static const struct adc_channel_cfg ch1_cfg = {
	.gain             = ADC_GAIN_1,
	.reference        = ADC_REF_VDD_1,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id       = 1U,
};
static const struct adc_channel_cfg ch2_cfg = {
	.gain             = ADC_GAIN_1,
	.reference        = ADC_REF_VDD_1,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id       = 2U,
};

static int16_t raw[2];
static const struct adc_sequence adc_seq = {
	.channels    = BIT(1) | BIT(2),
	.buffer      = raw,
	.buffer_size = sizeof(raw),
	.resolution  = 12U,
};

static float adc_to_celsius(int16_t adc_val)
{
	if (adc_val <= 0 || adc_val >= ADC_MAX_VAL) {
		return 25.0f; /* open/short protection */
	}
	float r_ntc = NTC_RSERIES * (float)adc_val / (float)(ADC_MAX_VAL - adc_val);
	float t_k = 1.0f / (logf(r_ntc / NTC_R0) / NTC_B + 1.0f / NTC_T0_K);

	return t_k - 273.15f;
}

int thermal_manager_init(void)
{
	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc1));
	if (!device_is_ready(adc_dev)) {
		LOG_ERR("ADC1 not ready");
		return -ENODEV;
	}
	adc_channel_setup(adc_dev, &ch1_cfg);
	adc_channel_setup(adc_dev, &ch2_cfg);
	LOG_INF("Thermal manager ready");
	return 0;
}

void thermal_manager_sample(void)
{
	int ret = adc_read(adc_dev, &adc_seq);

	if (ret) {
		LOG_WRN("ADC read error %d", ret);
		return;
	}

	float led_c    = adc_to_celsius(raw[0]);
	float driver_c = adc_to_celsius(raw[1]);
	float max_c    = (led_c > driver_c) ? led_c : driver_c;

	k_mutex_lock(&thermal_mutex, K_FOREVER);
	g_thermal.led_ntc_c         = led_c;
	g_thermal.driver_ntc_c      = driver_c;
	g_thermal.throttle_active   = (max_c >= THROTTLE_THRESHOLD_C);
	g_thermal.emergency_shutdown = (max_c >= SHUTDOWN_THRESHOLD_C);
	k_mutex_unlock(&thermal_mutex);

	if (g_thermal.emergency_shutdown) {
		LOG_ERR("THERMAL EMERGENCY: LED=%.1f DRIVER=%.1f", (double)led_c, (double)driver_c);
	} else if (g_thermal.throttle_active) {
		LOG_WRN("Thermal throttle: LED=%.1f DRIVER=%.1f", (double)led_c, (double)driver_c);
	}
}

void thermal_manager_get_state(struct thermal_state_t *out)
{
	k_mutex_lock(&thermal_mutex, K_FOREVER);
	*out = g_thermal;
	k_mutex_unlock(&thermal_mutex);
}
