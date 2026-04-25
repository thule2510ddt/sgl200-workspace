#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "feetech_servo.h"

LOG_MODULE_REGISTER(feetech, LOG_LEVEL_INF);

/* ── Feetech protocol constants ── */
#define INSTR_PING        0x01U
#define INSTR_READ_DATA   0x02U
#define INSTR_WRITE_DATA  0x03U
#define INSTR_TORQUE_EN   0x03U

/* Register addresses */
#define REG_TORQUE_EN          0x28U
#define REG_GOAL_POSITION_L    0x2AU
#define REG_PRESENT_POSITION_L 0x38U

/* Response header overhead: 0xFF 0xFF ID LEN ERR ... CHECKSUM */
#define RESP_OVERHEAD 6U

/* Bytes-on-wire delay at 1 Mbps: 1 byte = 10 µs.  Add 300 µs response latency. */
#define BYTE_TIME_US  10U
#define RESP_DELAY_US 300U

static const struct device *usart2;

/* Build and send a Feetech write packet */
static int send_packet(uint8_t id, uint8_t instr,
		       const uint8_t *params, uint8_t nparams)
{
	/* LEN = nparams + 2 (instr + checksum) */
	uint8_t len = nparams + 2U;
	uint8_t checksum = (uint8_t)(~(id + len + instr));

	for (uint8_t i = 0; i < nparams; i++) {
		checksum -= params[i];
	}

	uart_poll_out(usart2, 0xFFU);
	uart_poll_out(usart2, 0xFFU);
	uart_poll_out(usart2, id);
	uart_poll_out(usart2, len);
	uart_poll_out(usart2, instr);
	for (uint8_t i = 0; i < nparams; i++) {
		uart_poll_out(usart2, params[i]);
	}
	uart_poll_out(usart2, checksum);
	return 0;
}

/* Read response after send_packet.  Returns -ETIMEDOUT if no data arrives. */
static int read_response(uint8_t *buf, uint8_t expected_len, uint32_t timeout_us)
{
	for (uint8_t i = 0; i < expected_len; i++) {
		int64_t deadline = k_uptime_get() + (int64_t)(timeout_us / 1000U) + 1;
		int c;

		while (uart_poll_in(usart2, (unsigned char *)&c) != 0) {
			if (k_uptime_get() >= deadline) {
				return -ETIMEDOUT;
			}
		}
		buf[i] = (uint8_t)c;
	}
	return 0;
}

/* ── Public API ── */

int feetech_init(void)
{
	usart2 = DEVICE_DT_GET(DT_NODELABEL(usart2));
	if (!device_is_ready(usart2)) {
		LOG_ERR("USART2 not ready");
		return -ENODEV;
	}
	LOG_INF("Feetech driver ready on USART2");
	return 0;
}

int feetech_set_goal_position(uint8_t id, uint16_t pos)
{
	uint8_t params[3];

	params[0] = REG_GOAL_POSITION_L;
	params[1] = (uint8_t)(pos & 0xFFU);
	params[2] = (uint8_t)(pos >> 8);
	/* Write-only: skip response read */
	return send_packet(id, INSTR_WRITE_DATA, params, sizeof(params));
}

int feetech_get_position(uint8_t id, uint16_t *pos)
{
	uint8_t params[2] = { REG_PRESENT_POSITION_L, 2U };
	int ret;

	ret = send_packet(id, INSTR_READ_DATA, params, sizeof(params));
	if (ret) {
		return ret;
	}

	/* Wait for TX to complete + response latency */
	k_busy_wait((uint32_t)(7U * BYTE_TIME_US + RESP_DELAY_US));

	/* Response: 0xFF 0xFF ID 4 ERR POS_L POS_H CHECKSUM */
	uint8_t resp[8];

	ret = read_response(resp, sizeof(resp), 5000U);
	if (ret) {
		return ret;
	}
	if (resp[0] != 0xFFU || resp[1] != 0xFFU || resp[2] != id) {
		return -EIO;
	}
	*pos = (uint16_t)resp[5] | ((uint16_t)resp[6] << 8);
	return 0;
}

int feetech_torque_enable(uint8_t id, bool enable)
{
	uint8_t params[2] = { REG_TORQUE_EN, enable ? 1U : 0U };

	return send_packet(id, INSTR_WRITE_DATA, params, sizeof(params));
}

uint16_t feetech_angle_to_pos(float angle_deg, float min_deg, float max_deg)
{
	if (angle_deg < min_deg) {
		angle_deg = min_deg;
	}
	if (angle_deg > max_deg) {
		angle_deg = max_deg;
	}
	float pos_f = (float)FEETECH_POS_CENTER + (angle_deg * (float)FEETECH_POS_MAX / 360.0f);
	int32_t pos = (int32_t)pos_f;

	if (pos < 0) {
		return 0U;
	}
	if (pos > (int32_t)FEETECH_POS_MAX) {
		return (uint16_t)FEETECH_POS_MAX;
	}
	return (uint16_t)pos;
}
