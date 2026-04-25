#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Feetech half-duplex UART servo driver for ST3215HS on USART2 (PA2). */

/* Servo IDs as per hardware design */
#define FEETECH_ID_PITCH  1U
#define FEETECH_ID_YAW    2U

/*
 * Raw position range: 0–4095 maps to 0–360°.
 * Center (0° relative) = 2048.
 */
#define FEETECH_POS_CENTER  2048U
#define FEETECH_POS_MAX     4095U

int feetech_init(void);

/*
 * Set goal position.  pos is a raw servo count [0, 4095].
 * Write-only command; does not wait for servo echo.
 */
int feetech_set_goal_position(uint8_t id, uint16_t pos);

/* Read present position.  Blocks up to 5 ms for servo response. */
int feetech_get_position(uint8_t id, uint16_t *pos);

int feetech_torque_enable(uint8_t id, bool enable);

/* Convert ±180° angle (with hard limits applied) to servo counts. */
uint16_t feetech_angle_to_pos(float angle_deg, float min_deg, float max_deg);
