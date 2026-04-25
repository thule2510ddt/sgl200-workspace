#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/* ── IMU sample (local to imu_thread, consumed by Madgwick) ── */
struct imu_sample_t {
	int64_t timestamp_us;
	float accel_mps2[3];
	float gyro_dps[3];
	float temp_c;
};

/* ── Attitude state (written by imu_thread, read by control/angle/mavlink_tx) ── */
struct attitude_state_t {
	int64_t timestamp_us;
	float q[4];
	float euler_deg[3];
	float gyro_dps[3];
};

/* ── Gimbal mode ── */
enum gimbal_mode_t {
	GIMBAL_STABILIZE = 0,
	GIMBAL_FOLLOW,
	GIMBAL_ROI,
	GIMBAL_LOCK,
	GIMBAL_NEUTRAL,
};

/* ── Gimbal setpoint (written by mavlink_rx, read by angle_thread) ── */
struct gimbal_setpoint_t {
	int64_t timestamp_us;
	enum gimbal_mode_t mode;
	float pitch_deg;
	float yaw_deg;
	float q[4];
	uint16_t flags;
};

/* ── LED mode ── */
enum led_mode_t {
	LED_MODE_IDLE = 0,
	LED_MODE_NORMAL,
	LED_MODE_STROBE_WHITE,
	LED_MODE_STROBE_POLICE,
	LED_MODE_STROBE_SOS,
};

/* ── LED command (k_msgq payload, written by mavlink_rx) ── */
struct led_command_t {
	int64_t timestamp_us;
	enum led_mode_t mode;
	uint8_t brightness_pct;
};

/* ── LED FSM internal states ── */
enum led_fsm_state_t {
	LED_FSM_IDLE = 0,
	LED_FSM_SOFT_START,
	LED_FSM_NORMAL,
	LED_FSM_STROBE_WHITE,
	LED_FSM_STROBE_POLICE,
	LED_FSM_STROBE_SOS,
	LED_FSM_THERMAL_THROTTLE,
	LED_FSM_EMERGENCY_OFF,
};

/* ── Thermal state (written by thermal_thread, read by led_manager + mavlink_tx) ── */
struct thermal_state_t {
	float led_ntc_c;
	float driver_ntc_c;
	bool throttle_active;
	bool emergency_shutdown;
};

/* ── MAVLink context (written by mavlink_rx, read by mavlink_tx) ── */
struct mavlink_context_t {
	uint8_t target_system;
	uint8_t target_component;
	uint32_t last_heartbeat_ms;
	bool fc_armed;
	uint8_t tx_seq;
};

/* ── COMMAND_ACK event (queued from mavlink_rx, drained by mavlink_tx) ── */
struct ack_event_t {
	uint16_t command;
	uint8_t result;
	uint8_t target_system;
	uint8_t target_component;
};

/* ── Fault event (STATUSTEXT payload, queued from any thread) ── */
struct fault_event_t {
	char text[50];
	uint8_t severity;
};

/* ── IPC objects (defined in main.c) ── */
extern struct k_sem imu_data_ready;
extern struct k_sem rate_data_ready;
extern struct k_msgq led_cmd_queue;
extern struct k_msgq ack_queue;
extern struct k_msgq fault_queue;
extern struct k_mutex attitude_mutex;
extern struct k_mutex setpoint_mutex;
extern struct k_mutex thermal_mutex;
extern struct k_mutex mavlink_ctx_mutex;
extern struct k_mutex rate_sp_mutex;

/* ── Shared state (defined in main.c) ── */
extern struct attitude_state_t g_attitude;
extern struct gimbal_setpoint_t g_setpoint;
extern struct thermal_state_t g_thermal;
extern struct mavlink_context_t g_mavlink_ctx;
extern float g_pitch_rate_sp;
extern float g_yaw_rate_sp;
