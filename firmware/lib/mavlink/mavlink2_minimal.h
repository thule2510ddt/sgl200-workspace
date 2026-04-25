#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* ── MAVLink v2 constants ── */
#define MAVLINK2_STX              0xFDU
#define MAVLINK2_HEADER_LEN       10U
#define MAVLINK2_CHECKSUM_LEN     2U
#define MAVLINK2_MAX_PAYLOAD_LEN  255U
#define MAVLINK2_MAX_FRAME_LEN    (MAVLINK2_HEADER_LEN + MAVLINK2_MAX_PAYLOAD_LEN + MAVLINK2_CHECKSUM_LEN)

/* ── System / component IDs ── */
#define SGL200_SYSTEM_ID          1U
#define SGL200_COMPONENT_ID       154U   /* MAV_COMP_ID_GIMBAL */

/* ── Message IDs ── */
#define MAVLINK_MSG_ID_HEARTBEAT                    0UL
#define MAVLINK_MSG_ID_PARAM_VALUE                  22UL
#define MAVLINK_MSG_ID_PARAM_SET                    23UL
#define MAVLINK_MSG_ID_COMMAND_LONG                 76UL
#define MAVLINK_MSG_ID_COMMAND_ACK                  77UL
#define MAVLINK_MSG_ID_GIMBAL_DEVICE_INFORMATION    283UL
#define MAVLINK_MSG_ID_GIMBAL_DEVICE_SET_ATTITUDE   284UL
#define MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS 285UL

/* ── MAVLink enums (subset) ── */
#define MAV_TYPE_GIMBAL           26U
#define MAV_AUTOPILOT_INVALID     8U
#define MAV_STATE_ACTIVE          4U
#define MAV_STATE_STANDBY         3U
#define MAV_MODE_FLAG_SAFETY_ARMED 0x80U
#define MAV_RESULT_ACCEPTED       0U
#define MAV_RESULT_DENIED         1U
#define MAV_RESULT_UNSUPPORTED    3U
#define MAV_SEVERITY_CRITICAL     2U
#define MAV_SEVERITY_WARNING      4U
#define MAV_SEVERITY_INFO         6U

/* ── GIMBAL_DEVICE flags (subset) ── */
#define GIMBAL_DEVICE_FLAGS_RETRACT          0x0001U
#define GIMBAL_DEVICE_FLAGS_NEUTRAL          0x0002U
#define GIMBAL_DEVICE_FLAGS_ROLL_LOCK        0x0004U
#define GIMBAL_DEVICE_FLAGS_PITCH_LOCK       0x0008U
#define GIMBAL_DEVICE_FLAGS_YAW_LOCK         0x0010U

/* ── Parsed frame ── */
typedef struct {
	uint32_t msg_id;
	uint8_t  sys_id;
	uint8_t  comp_id;
	uint8_t  payload[MAVLINK2_MAX_PAYLOAD_LEN];
	uint8_t  payload_len;
} mavlink2_frame_t;

/* ── Byte-level parser state ── */
typedef struct {
	uint8_t  buf[MAVLINK2_MAX_FRAME_LEN];
	uint16_t pos;
	uint8_t  payload_len;
} mavlink2_parser_t;

/* ── Encoder: returns total frame length, or -1 on error ── */
int mavlink2_encode_heartbeat(uint8_t *buf, uint8_t seq,
			      uint8_t sys_id, uint8_t comp_id,
			      bool armed);

int mavlink2_encode_gimbal_attitude_status(uint8_t *buf, uint8_t seq,
					   uint8_t sys_id, uint8_t comp_id,
					   uint8_t target_sys, uint8_t target_comp,
					   const float q[4],
					   float wx, float wy, float wz,
					   uint32_t failure_flags, uint16_t flags);

int mavlink2_encode_command_ack(uint8_t *buf, uint8_t seq,
				uint8_t sys_id, uint8_t comp_id,
				uint16_t cmd, uint8_t result,
				uint8_t target_sys, uint8_t target_comp);

int mavlink2_encode_param_value(uint8_t *buf, uint8_t seq,
				uint8_t sys_id, uint8_t comp_id,
				const char *name, float value,
				uint16_t index, uint16_t count);

int mavlink2_encode_statustext(uint8_t *buf, uint8_t seq,
			       uint8_t sys_id, uint8_t comp_id,
			       uint8_t severity, const char *text);

/* ── Parser: returns 1 when a complete valid frame is ready in *out ── */
int mavlink2_parse_byte(mavlink2_parser_t *p, uint8_t byte, mavlink2_frame_t *out);

/* ── Decoders ── */
void mavlink2_decode_heartbeat(const mavlink2_frame_t *f,
			       uint8_t *base_mode, uint8_t *system_status);

void mavlink2_decode_gimbal_set_attitude(const mavlink2_frame_t *f,
					 float q[4], float rates[3],
					 uint16_t *flags,
					 uint8_t *target_sys,
					 uint8_t *target_comp);

void mavlink2_decode_command_long(const mavlink2_frame_t *f,
				  uint16_t *cmd, float params[7],
				  uint8_t *target_sys, uint8_t *target_comp);

void mavlink2_decode_param_set(const mavlink2_frame_t *f,
			       char name[17], float *value,
			       uint8_t *target_sys, uint8_t *target_comp);
