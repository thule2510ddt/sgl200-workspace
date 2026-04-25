#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include "mavlink_agent.h"
#include "sgl200_types.h"
#include "mavlink2_minimal.h"

LOG_MODULE_REGISTER(mavlink, LOG_LEVEL_INF);

/* ── MAVLink custom LED command ID (vendor-specific) ── */
#define MAV_CMD_DO_LED_CONTROL  31000U

/* ── Ring buffer for async UART RX ── */
#define RX_RING_SIZE 512U
RING_BUF_DECLARE(rx_ring, RX_RING_SIZE);

/* ── TX done semaphore ── */
static K_SEM_DEFINE(tx_done_sem, 0, 1);
static K_SEM_DEFINE(rx_avail_sem, 0, 1);

static const struct device *usart3;
static uint8_t rx_dma_buf[256];
static uint8_t tx_frame_buf[MAVLINK2_MAX_FRAME_LEN];

/* ── UART async callback ── */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		k_sem_give(&tx_done_sem);
		break;

	case UART_RX_RDY:
		ring_buf_put(&rx_ring,
			     &evt->data.rx.buf[evt->data.rx.offset],
			     evt->data.rx.len);
		k_sem_give(&rx_avail_sem);
		break;

	case UART_RX_BUF_REQUEST:
		break;

	case UART_RX_BUF_RELEASED:
		break;

	case UART_RX_DISABLED:
		/* Re-enable RX after stop event */
		uart_rx_enable(usart3, rx_dma_buf, sizeof(rx_dma_buf), 100000U);
		break;

	default:
		break;
	}
}

static int mavlink_send(const uint8_t *buf, size_t len)
{
	int ret = uart_tx(usart3, buf, len, SYS_FOREVER_US);

	if (ret) {
		return ret;
	}
	return k_sem_take(&tx_done_sem, K_MSEC(50));
}

/* ── Command handlers ── */
static void handle_heartbeat(const mavlink2_frame_t *f)
{
	uint8_t base_mode, sys_status;

	mavlink2_decode_heartbeat(f, &base_mode, &sys_status);

	k_mutex_lock(&mavlink_ctx_mutex, K_FOREVER);
	g_mavlink_ctx.target_system    = f->sys_id;
	g_mavlink_ctx.target_component = f->comp_id;
	g_mavlink_ctx.last_heartbeat_ms = k_uptime_get_32();
	g_mavlink_ctx.fc_armed = (base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0U;
	k_mutex_unlock(&mavlink_ctx_mutex);
}

static void handle_gimbal_set_attitude(const mavlink2_frame_t *f)
{
	float q[4], rates[3];
	uint16_t flags;
	uint8_t target_sys, target_comp;

	mavlink2_decode_gimbal_set_attitude(f, q, rates, &flags,
					    &target_sys, &target_comp);

	if (target_sys != SGL200_SYSTEM_ID || target_comp != SGL200_COMPONENT_ID) {
		return;
	}

	k_mutex_lock(&setpoint_mutex, K_FOREVER);
	g_setpoint.timestamp_us = k_ticks_to_us_floor64(k_uptime_ticks());
	g_setpoint.flags = flags;

	if (flags & GIMBAL_DEVICE_FLAGS_NEUTRAL) {
		g_setpoint.mode      = GIMBAL_NEUTRAL;
		g_setpoint.pitch_deg = 0.0f;
		g_setpoint.yaw_deg   = 0.0f;
	} else if (flags & GIMBAL_DEVICE_FLAGS_RETRACT) {
		g_setpoint.mode      = GIMBAL_NEUTRAL;
		g_setpoint.pitch_deg = 0.0f;
		g_setpoint.yaw_deg   = 0.0f;
	} else {
		g_setpoint.mode = GIMBAL_STABILIZE;
		/* Convert quaternion to Euler only if all q values are not NaN */
		if (q[0] == q[0]) { /* NaN check */
			g_setpoint.q[0] = q[0];
			g_setpoint.q[1] = q[1];
			g_setpoint.q[2] = q[2];
			g_setpoint.q[3] = q[3];
		}
	}
	k_mutex_unlock(&setpoint_mutex);

	/* Queue ACK */
	struct ack_event_t ack = {
		.command        = (uint16_t)MAVLINK_MSG_ID_GIMBAL_DEVICE_SET_ATTITUDE,
		.result         = MAV_RESULT_ACCEPTED,
		.target_system  = f->sys_id,
		.target_component = f->comp_id,
	};
	k_msgq_put(&ack_queue, &ack, K_NO_WAIT);
}

static void handle_command_long(const mavlink2_frame_t *f)
{
	uint16_t cmd;
	float params[7];
	uint8_t target_sys, target_comp;

	mavlink2_decode_command_long(f, &cmd, params, &target_sys, &target_comp);

	if (target_sys != SGL200_SYSTEM_ID || target_comp != SGL200_COMPONENT_ID) {
		return;
	}

	uint8_t result = MAV_RESULT_UNSUPPORTED;

	if (cmd == MAV_CMD_DO_LED_CONTROL) {
		struct led_command_t led_cmd = {
			.timestamp_us  = k_ticks_to_us_floor64(k_uptime_ticks()),
			.mode          = (enum led_mode_t)(uint8_t)params[0],
			.brightness_pct = (uint8_t)params[1],
		};
		k_msgq_put(&led_cmd_queue, &led_cmd, K_NO_WAIT);
		result = MAV_RESULT_ACCEPTED;
	}

	struct ack_event_t ack = {
		.command          = cmd,
		.result           = result,
		.target_system    = f->sys_id,
		.target_component = f->comp_id,
	};
	k_msgq_put(&ack_queue, &ack, K_NO_WAIT);
}

/* ── Public API ── */

int mavlink_agent_init(void)
{
	usart3 = DEVICE_DT_GET(DT_NODELABEL(usart3));
	if (!device_is_ready(usart3)) {
		LOG_ERR("USART3 not ready");
		return -ENODEV;
	}
	uart_callback_set(usart3, uart_cb, NULL);
	uart_rx_enable(usart3, rx_dma_buf, sizeof(rx_dma_buf), 100000U);
	LOG_INF("MAVLink agent ready on USART3");
	return 0;
}

void mavlink_rx_run(void)
{
	mavlink2_parser_t parser = { 0 };
	mavlink2_frame_t  frame;

	while (true) {
		k_sem_take(&rx_avail_sem, K_FOREVER);

		uint8_t byte;

		while (ring_buf_get(&rx_ring, &byte, 1U) == 1U) {
			if (mavlink2_parse_byte(&parser, byte, &frame)) {
				switch (frame.msg_id) {
				case MAVLINK_MSG_ID_HEARTBEAT:
					handle_heartbeat(&frame);
					break;
				case MAVLINK_MSG_ID_GIMBAL_DEVICE_SET_ATTITUDE:
					handle_gimbal_set_attitude(&frame);
					break;
				case MAVLINK_MSG_ID_COMMAND_LONG:
					handle_command_long(&frame);
					break;
				default:
					break;
				}
			}
		}
	}
}

void mavlink_tx_run(void)
{
	uint8_t loop_count = 0U;
	struct mavlink_context_t ctx;
	struct attitude_state_t att;
	struct ack_event_t ack;
	struct fault_event_t fault;

	while (true) {
		k_sleep(K_MSEC(250));
		loop_count++;

		k_mutex_lock(&mavlink_ctx_mutex, K_FOREVER);
		ctx = g_mavlink_ctx;
		k_mutex_unlock(&mavlink_ctx_mutex);

		k_mutex_lock(&attitude_mutex, K_FOREVER);
		att = g_attitude;
		k_mutex_unlock(&attitude_mutex);

		/* HEARTBEAT at 1 Hz (every 4th loop) */
		if (loop_count % 4U == 0U) {
			int len = mavlink2_encode_heartbeat(
				tx_frame_buf, ctx.tx_seq++,
				SGL200_SYSTEM_ID, SGL200_COMPONENT_ID,
				ctx.fc_armed);
			mavlink_send(tx_frame_buf, (size_t)len);
		}

		/* GIMBAL_DEVICE_ATTITUDE_STATUS at 4 Hz */
		{
			uint16_t flags = GIMBAL_DEVICE_FLAGS_PITCH_LOCK |
					 GIMBAL_DEVICE_FLAGS_YAW_LOCK;
			int len = mavlink2_encode_gimbal_attitude_status(
				tx_frame_buf, ctx.tx_seq++,
				SGL200_SYSTEM_ID, SGL200_COMPONENT_ID,
				ctx.target_system, ctx.target_component,
				att.q,
				att.gyro_dps[0], att.gyro_dps[1], att.gyro_dps[2],
				0U, flags);
			mavlink_send(tx_frame_buf, (size_t)len);
		}

		/* Drain ACK queue (up to 4 per loop) */
		for (int i = 0; i < 4; i++) {
			if (k_msgq_get(&ack_queue, &ack, K_NO_WAIT) != 0) {
				break;
			}
			int len = mavlink2_encode_command_ack(
				tx_frame_buf, ctx.tx_seq++,
				SGL200_SYSTEM_ID, SGL200_COMPONENT_ID,
				ack.command, ack.result,
				ack.target_system, ack.target_component);
			mavlink_send(tx_frame_buf, (size_t)len);
		}

		/* Drain fault queue (one STATUSTEXT per loop) */
		if (k_msgq_get(&fault_queue, &fault, K_NO_WAIT) == 0) {
			int len = mavlink2_encode_statustext(
				tx_frame_buf, ctx.tx_seq++,
				SGL200_SYSTEM_ID, SGL200_COMPONENT_ID,
				fault.severity, fault.text);
			mavlink_send(tx_frame_buf, (size_t)len);
		}
	}
}
