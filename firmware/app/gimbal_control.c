#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "gimbal_control.h"
#include "pid.h"
#include "feetech_servo.h"

LOG_MODULE_REGISTER(gimbal, LOG_LEVEL_INF);

/* Angle limits (deg) */
#define PITCH_MIN_DEG  (-90.0f)
#define PITCH_MAX_DEG  ( 30.0f)
#define YAW_MIN_DEG    (-160.0f)
#define YAW_MAX_DEG    ( 160.0f)

/* Rate setpoint limits (deg/s) — outer loop output clamped here */
#define RATE_LIMIT_DPS  500.0f

/* PID initial gains (from 04-ARCHITECTURE.md) */
static pid_t pitch_angle_pid;
static pid_t pitch_rate_pid;
static pid_t yaw_angle_pid;
static pid_t yaw_rate_pid;

/* Locked angle when GIMBAL_LOCK mode activates */
static float lock_pitch_deg;
static float lock_yaw_deg;
static bool  lock_captured;

int gimbal_control_init(void)
{
	pid_init(&pitch_angle_pid, 8.0f, 0.5f, 0.1f,  50.0f, RATE_LIMIT_DPS);
	pid_init(&pitch_rate_pid,  0.8f, 0.02f, 0.005f, 10.0f, 2000.0f);
	pid_init(&yaw_angle_pid,   6.0f, 0.3f, 0.08f,  50.0f, RATE_LIMIT_DPS);
	pid_init(&yaw_rate_pid,    0.6f, 0.015f, 0.003f, 10.0f, 2000.0f);

	lock_captured = false;
	LOG_INF("Gimbal control ready");
	return 0;
}

void gimbal_reset(void)
{
	pid_reset(&pitch_angle_pid);
	pid_reset(&pitch_rate_pid);
	pid_reset(&yaw_angle_pid);
	pid_reset(&yaw_rate_pid);
	lock_captured = false;
}

void gimbal_angle_tick(const struct attitude_state_t *att, float dt)
{
	struct gimbal_setpoint_t sp;

	k_mutex_lock(&setpoint_mutex, K_FOREVER);
	sp = g_setpoint;
	k_mutex_unlock(&setpoint_mutex);

	float target_pitch, target_yaw;

	switch (sp.mode) {
	case GIMBAL_STABILIZE:
		target_pitch = sp.pitch_deg;
		target_yaw   = sp.yaw_deg;
		lock_captured = false;
		break;

	case GIMBAL_FOLLOW:
		/* Yaw follows drone heading; extract yaw from attitude quaternion */
		target_pitch = sp.pitch_deg;
		target_yaw   = att->euler_deg[2]; /* drone yaw */
		lock_captured = false;
		break;

	case GIMBAL_LOCK:
		if (!lock_captured) {
			lock_pitch_deg = att->euler_deg[1];
			lock_yaw_deg   = att->euler_deg[2];
			lock_captured  = true;
		}
		target_pitch = lock_pitch_deg;
		target_yaw   = lock_yaw_deg;
		break;

	case GIMBAL_ROI:
		/* ROI angle computation placeholder — use stored setpoint */
		target_pitch = sp.pitch_deg;
		target_yaw   = sp.yaw_deg;
		lock_captured = false;
		break;

	case GIMBAL_NEUTRAL:
	default:
		target_pitch  = 0.0f;
		target_yaw    = 0.0f;
		lock_captured = false;
		break;
	}

	float pitch_rate_sp = pid_compute(&pitch_angle_pid, target_pitch,
					  att->euler_deg[1], dt);
	float yaw_rate_sp   = pid_compute(&yaw_angle_pid,   target_yaw,
					  att->euler_deg[2], dt);

	k_mutex_lock(&rate_sp_mutex, K_FOREVER);
	g_pitch_rate_sp = pitch_rate_sp;
	g_yaw_rate_sp   = yaw_rate_sp;
	k_mutex_unlock(&rate_sp_mutex);
}

void gimbal_rate_tick(const struct attitude_state_t *att, float dt)
{
	float pitch_rate_sp, yaw_rate_sp;

	k_mutex_lock(&rate_sp_mutex, K_FOREVER);
	pitch_rate_sp = g_pitch_rate_sp;
	yaw_rate_sp   = g_yaw_rate_sp;
	k_mutex_unlock(&rate_sp_mutex);

	float pitch_out = pid_compute(&pitch_rate_pid, pitch_rate_sp,
				      att->gyro_dps[1], dt);
	float yaw_out   = pid_compute(&yaw_rate_pid,   yaw_rate_sp,
				      att->gyro_dps[2], dt);

	/*
	 * Convert PID output (rate-loop) to absolute servo angle.
	 * The rate PID output is treated as a delta-position command in deg.
	 * We maintain a running target angle and clamp to joint limits.
	 */
	static float pitch_servo_deg;
	static float yaw_servo_deg;

	pitch_servo_deg += pitch_out * dt;
	yaw_servo_deg   += yaw_out   * dt;

	if (pitch_servo_deg < PITCH_MIN_DEG) { pitch_servo_deg = PITCH_MIN_DEG; }
	if (pitch_servo_deg > PITCH_MAX_DEG) { pitch_servo_deg = PITCH_MAX_DEG; }
	if (yaw_servo_deg   < YAW_MIN_DEG)   { yaw_servo_deg   = YAW_MIN_DEG;   }
	if (yaw_servo_deg   > YAW_MAX_DEG)   { yaw_servo_deg   = YAW_MAX_DEG;   }

	uint16_t pitch_pos = feetech_angle_to_pos(pitch_servo_deg, PITCH_MIN_DEG, PITCH_MAX_DEG);
	uint16_t yaw_pos   = feetech_angle_to_pos(yaw_servo_deg,   YAW_MIN_DEG,   YAW_MAX_DEG);

	feetech_set_goal_position(FEETECH_ID_PITCH, pitch_pos);
	feetech_set_goal_position(FEETECH_ID_YAW,   yaw_pos);
}
