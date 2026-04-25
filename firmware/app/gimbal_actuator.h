#pragma once

enum gimbal_axis_t {
	GIMBAL_AXIS_PITCH = 0,
	GIMBAL_AXIS_YAW,
};

int gimbal_actuator_init(void);
int gimbal_actuator_set_angle(enum gimbal_axis_t axis, float angle_deg);
