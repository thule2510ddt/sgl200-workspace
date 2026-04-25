#pragma once

#include "sgl200_types.h"

int  led_manager_init(void);
void led_manager_tick(const struct led_command_t *cmd,     /* NULL = keep current */
		      const struct thermal_state_t *thermal,
		      bool fc_armed);
