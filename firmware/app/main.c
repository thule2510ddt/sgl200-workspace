#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "sgl200_types.h"
#include "icm42688.h"
#include "feetech_servo.h"
#include "madgwick_ahrs.h"
#include "thermal_manager.h"
#include "led_manager.h"
#include "gimbal_control.h"
#include "mavlink_agent.h"

LOG_MODULE_REGISTER(sgl200, LOG_LEVEL_INF);

/* ── Stack sizes ── */
#define IMU_THREAD_STACK_SIZE      2048
#define CONTROL_THREAD_STACK_SIZE  2048
#define ANGLE_THREAD_STACK_SIZE    1536
#define MAVLINK_RX_STACK_SIZE      2048
#define MAVLINK_TX_STACK_SIZE      2048
#define LED_MANAGER_STACK_SIZE     1536
#define THERMAL_THREAD_STACK_SIZE  1536
#define WATCHDOG_THREAD_STACK_SIZE 1024

/* ── Thread priorities (0 = highest preemptive) ── */
#define IMU_THREAD_PRIORITY        0
#define CONTROL_THREAD_PRIORITY    1
#define ANGLE_THREAD_PRIORITY      2
#define MAVLINK_RX_THREAD_PRIORITY 3
#define MAVLINK_TX_THREAD_PRIORITY 4
#define LED_THREAD_PRIORITY        5
#define THERMAL_THREAD_PRIORITY    6
#define WATCHDOG_THREAD_PRIORITY   7

/* ── IPC definitions ── */
K_SEM_DEFINE(imu_data_ready,  0, 1);
K_SEM_DEFINE(rate_data_ready, 0, 1);

K_MSGQ_DEFINE(led_cmd_queue, sizeof(struct led_command_t),  8, 4);
K_MSGQ_DEFINE(ack_queue,     sizeof(struct ack_event_t),    4, 4);
K_MSGQ_DEFINE(fault_queue,   sizeof(struct fault_event_t),  4, 4);

K_MUTEX_DEFINE(attitude_mutex);
K_MUTEX_DEFINE(setpoint_mutex);
K_MUTEX_DEFINE(thermal_mutex);
K_MUTEX_DEFINE(mavlink_ctx_mutex);
K_MUTEX_DEFINE(rate_sp_mutex);

/* ── Shared state ── */
struct attitude_state_t   g_attitude;
struct gimbal_setpoint_t  g_setpoint;
struct thermal_state_t    g_thermal;
struct mavlink_context_t  g_mavlink_ctx;
float g_pitch_rate_sp;
float g_yaw_rate_sp;

/* ── AHRS instance (owned by imu_thread) ── */
static madgwick_t ahrs;

/* ── Watchdog ── */
static const struct device *wdt_dev;
static int wdt_channel;

/* ──────────────────────────────────────────────
 * IMU thread  (priority 0, driven by INT ISR)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(imu_thread_stack, IMU_THREAD_STACK_SIZE);
static struct k_thread imu_thread_data;

static void imu_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	if (icm42688_init(&imu_data_ready) != 0) {
		LOG_ERR("ICM-42688-P init failed");
		return;
	}
	madgwick_init(&ahrs, 0.1f, 0.001f);

	while (true) {
		/* Block until ICM INT fires (1 kHz) */
		k_sem_take(&imu_data_ready, K_MSEC(5));

		struct imu_sample_t sample;

		if (icm42688_read_sample(&sample) != 0) {
			continue;
		}

		madgwick_update(&ahrs,
				sample.gyro_dps[0], sample.gyro_dps[1], sample.gyro_dps[2],
				sample.accel_mps2[0], sample.accel_mps2[1], sample.accel_mps2[2]);

		k_mutex_lock(&attitude_mutex, K_FOREVER);
		g_attitude.timestamp_us = sample.timestamp_us;
		g_attitude.q[0] = ahrs.q[0];
		g_attitude.q[1] = ahrs.q[1];
		g_attitude.q[2] = ahrs.q[2];
		g_attitude.q[3] = ahrs.q[3];
		madgwick_get_euler(&ahrs,
				   &g_attitude.euler_deg[0],
				   &g_attitude.euler_deg[1],
				   &g_attitude.euler_deg[2]);
		g_attitude.gyro_dps[0] = sample.gyro_dps[0];
		g_attitude.gyro_dps[1] = sample.gyro_dps[1];
		g_attitude.gyro_dps[2] = sample.gyro_dps[2];
		k_mutex_unlock(&attitude_mutex);

		/* Signal rate control loop */
		k_sem_give(&rate_data_ready);
	}
}

/* ──────────────────────────────────────────────
 * Control thread  (priority 1, 1 kHz rate PID)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(control_thread_stack, CONTROL_THREAD_STACK_SIZE);
static struct k_thread control_thread_data;

static void control_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sem_take(&rate_data_ready, K_MSEC(5));

		struct attitude_state_t att;

		k_mutex_lock(&attitude_mutex, K_FOREVER);
		att = g_attitude;
		k_mutex_unlock(&attitude_mutex);

		gimbal_rate_tick(&att, 0.001f);
	}
}

/* ──────────────────────────────────────────────
 * Angle thread  (priority 2, 200 Hz angle PID)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(angle_thread_stack, ANGLE_THREAD_STACK_SIZE);
static struct k_thread angle_thread_data;

static void angle_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sleep(K_MSEC(5));

		struct attitude_state_t att;

		k_mutex_lock(&attitude_mutex, K_FOREVER);
		att = g_attitude;
		k_mutex_unlock(&attitude_mutex);

		gimbal_angle_tick(&att, 0.005f);
	}
}

/* ──────────────────────────────────────────────
 * MAVLink RX thread  (priority 3, async)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(mavlink_rx_thread_stack, MAVLINK_RX_STACK_SIZE);
static struct k_thread mavlink_rx_thread_data;

static void mavlink_rx_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	mavlink_rx_run();
}

/* ──────────────────────────────────────────────
 * MAVLink TX thread  (priority 4, 250 ms)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(mavlink_tx_thread_stack, MAVLINK_TX_STACK_SIZE);
static struct k_thread mavlink_tx_thread_data;

static void mavlink_tx_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	mavlink_tx_run();
}

/* ──────────────────────────────────────────────
 * LED manager thread  (priority 5, 10 ms FSM)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(led_thread_stack, LED_MANAGER_STACK_SIZE);
static struct k_thread led_thread_data;

static void led_manager_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sleep(K_MSEC(10));

		struct led_command_t cmd;
		struct led_command_t *cmd_ptr = NULL;
		struct thermal_state_t thermal;
		bool fc_armed;

		if (k_msgq_get(&led_cmd_queue, &cmd, K_NO_WAIT) == 0) {
			cmd_ptr = &cmd;
		}

		thermal_manager_get_state(&thermal);

		k_mutex_lock(&mavlink_ctx_mutex, K_FOREVER);
		fc_armed = g_mavlink_ctx.fc_armed;
		k_mutex_unlock(&mavlink_ctx_mutex);

		led_manager_tick(cmd_ptr, &thermal, fc_armed);
	}
}

/* ──────────────────────────────────────────────
 * Thermal thread  (priority 6, 100 ms)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(thermal_thread_stack, THERMAL_THREAD_STACK_SIZE);
static struct k_thread thermal_thread_data;

static void thermal_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sleep(K_MSEC(100));
		thermal_manager_sample();
	}
}

/* ──────────────────────────────────────────────
 * Watchdog thread  (priority 7, 400 ms)
 * ────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(watchdog_thread_stack, WATCHDOG_THREAD_STACK_SIZE);
static struct k_thread watchdog_thread_data;

static void watchdog_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sleep(K_MSEC(400));

		/* Feed IWDG */
		if (wdt_channel >= 0) {
			wdt_feed(wdt_dev, wdt_channel);
		}

		/* FC heartbeat timeout → safe state */
		uint32_t now_ms = k_uptime_get_32();
		uint32_t last_hb;

		k_mutex_lock(&mavlink_ctx_mutex, K_FOREVER);
		last_hb = g_mavlink_ctx.last_heartbeat_ms;
		k_mutex_unlock(&mavlink_ctx_mutex);

		uint32_t elapsed_ms = now_ms - last_hb;

		if (elapsed_ms > 3000U) {
			LOG_WRN("FC heartbeat timeout: %u ms — safe state", elapsed_ms);

			/* Safe state: gimbal neutral, LED at 50% */
			k_mutex_lock(&setpoint_mutex, K_FOREVER);
			g_setpoint.mode      = GIMBAL_NEUTRAL;
			g_setpoint.pitch_deg = 0.0f;
			g_setpoint.yaw_deg   = 0.0f;
			k_mutex_unlock(&setpoint_mutex);
		}
	}
}

/* ──────────────────────────────────────────────
 * Startup helpers
 * ────────────────────────────────────────────── */
static int init_watchdog(void)
{
	wdt_dev = DEVICE_DT_GET(DT_NODELABEL(iwdg));
	if (!device_is_ready(wdt_dev)) {
		LOG_WRN("IWDG not ready");
		wdt_channel = -1;
		return -ENODEV;
	}

	struct wdt_timeout_cfg wdt_cfg = {
		.window = { .min = 0U, .max = 1600U },
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC,
	};

	wdt_channel = wdt_install_timeout(wdt_dev, &wdt_cfg);
	if (wdt_channel < 0) {
		return wdt_channel;
	}
	return wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
}

static void start_threads(void)
{
	k_tid_t tid;

	tid = k_thread_create(&imu_thread_data, imu_thread_stack,
			      K_THREAD_STACK_SIZEOF(imu_thread_stack),
			      imu_thread, NULL, NULL, NULL,
			      IMU_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "imu_thread");

	tid = k_thread_create(&control_thread_data, control_thread_stack,
			      K_THREAD_STACK_SIZEOF(control_thread_stack),
			      control_thread, NULL, NULL, NULL,
			      CONTROL_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "control_thread");

	tid = k_thread_create(&angle_thread_data, angle_thread_stack,
			      K_THREAD_STACK_SIZEOF(angle_thread_stack),
			      angle_thread, NULL, NULL, NULL,
			      ANGLE_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "angle_thread");

	tid = k_thread_create(&mavlink_rx_thread_data, mavlink_rx_thread_stack,
			      K_THREAD_STACK_SIZEOF(mavlink_rx_thread_stack),
			      mavlink_rx_thread, NULL, NULL, NULL,
			      MAVLINK_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "mavlink_rx_thread");

	tid = k_thread_create(&mavlink_tx_thread_data, mavlink_tx_thread_stack,
			      K_THREAD_STACK_SIZEOF(mavlink_tx_thread_stack),
			      mavlink_tx_thread, NULL, NULL, NULL,
			      MAVLINK_TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "mavlink_tx_thread");

	tid = k_thread_create(&led_thread_data, led_thread_stack,
			      K_THREAD_STACK_SIZEOF(led_thread_stack),
			      led_manager_thread, NULL, NULL, NULL,
			      LED_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "led_manager_thread");

	tid = k_thread_create(&thermal_thread_data, thermal_thread_stack,
			      K_THREAD_STACK_SIZEOF(thermal_thread_stack),
			      thermal_thread, NULL, NULL, NULL,
			      THERMAL_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "thermal_thread");

	tid = k_thread_create(&watchdog_thread_data, watchdog_thread_stack,
			      K_THREAD_STACK_SIZEOF(watchdog_thread_stack),
			      watchdog_thread, NULL, NULL, NULL,
			      WATCHDOG_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "watchdog_thread");
}

/* ──────────────────────────────────────────────
 * main
 * ────────────────────────────────────────────── */
int main(void)
{
	LOG_INF("SGL-200 firmware boot");

	/* Seed FC heartbeat timestamp so watchdog doesn't fire immediately */
	k_mutex_lock(&mavlink_ctx_mutex, K_FOREVER);
	g_mavlink_ctx.last_heartbeat_ms = k_uptime_get_32();
	k_mutex_unlock(&mavlink_ctx_mutex);

	/* Default gimbal setpoint: neutral */
	k_mutex_lock(&setpoint_mutex, K_FOREVER);
	g_setpoint.mode      = GIMBAL_NEUTRAL;
	g_setpoint.pitch_deg = 0.0f;
	g_setpoint.yaw_deg   = 0.0f;
	k_mutex_unlock(&setpoint_mutex);

	/* Initialise subsystems in order: drivers first, then app layers */
	int ret;

	ret = feetech_init();
	if (ret) {
		LOG_ERR("Feetech init failed: %d", ret);
	}

	ret = thermal_manager_init();
	if (ret) {
		LOG_ERR("Thermal manager init failed: %d", ret);
	}

	ret = led_manager_init();
	if (ret) {
		LOG_ERR("LED manager init failed: %d", ret);
	}

	ret = gimbal_control_init();
	if (ret) {
		LOG_ERR("Gimbal control init failed: %d", ret);
	}

	ret = mavlink_agent_init();
	if (ret) {
		LOG_ERR("MAVLink agent init failed: %d", ret);
	}

	ret = init_watchdog();
	if (ret) {
		LOG_WRN("Watchdog init failed: %d — continuing without IWDG", ret);
	}

	/* ICM-42688-P is initialised inside imu_thread after scheduler starts */

	LOG_INF("Starting threads");
	start_threads();

	/* main() becomes idle — all work is in threads */
	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
