#pragma once

#include "sgl200_types.h"

int  thermal_manager_init(void);
void thermal_manager_sample(void);                          /* call every 100 ms */
void thermal_manager_get_state(struct thermal_state_t *out); /* reads under mutex */
