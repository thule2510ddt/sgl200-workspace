#include <errno.h>
#include <zephyr/kernel.h>
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
	LOG_ERR("Gimbal actuator servo PWM backend is not implemented yet");
	return -ENOTSUP;
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
	ARG_UNUSED(axis);
	ARG_UNUSED(angle_deg);
	return -ENOTSUP;
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
