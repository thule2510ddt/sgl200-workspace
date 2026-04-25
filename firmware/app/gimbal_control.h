#pragma once

#include "sgl200_types.h"

int  gimbal_control_init(void);
void gimbal_angle_tick(const struct attitude_state_t *att, float dt); /* 200 Hz */
void gimbal_rate_tick(const struct attitude_state_t *att, float dt);  /* 1 kHz  */
void gimbal_reset(void);
