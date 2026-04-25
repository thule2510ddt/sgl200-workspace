#include "mavlink2_minimal.h"

/* CRC_EXTRA bytes per message ID (X.25 CRC seed = 0xFFFF) */
static const struct {
	uint32_t msg_id;
	uint8_t  crc_extra;
} crc_extra_table[] = {
	{ MAVLINK_MSG_ID_HEARTBEAT,                    50U  },
	{ MAVLINK_MSG_ID_PARAM_VALUE,                  220U },
	{ MAVLINK_MSG_ID_PARAM_SET,                    168U },
	{ MAVLINK_MSG_ID_COMMAND_LONG,                 152U },
	{ MAVLINK_MSG_ID_COMMAND_ACK,                  143U },
	{ MAVLINK_MSG_ID_GIMBAL_DEVICE_INFORMATION,    74U  },
	{ MAVLINK_MSG_ID_GIMBAL_DEVICE_SET_ATTITUDE,   99U  },
	{ MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS, 137U },
};

static uint8_t get_crc_extra(uint32_t msg_id)
{
	for (size_t i = 0; i < sizeof(crc_extra_table) / sizeof(crc_extra_table[0]); i++) {
		if (crc_extra_table[i].msg_id == msg_id) {
			return crc_extra_table[i].crc_extra;
		}
	}
	return 0U;
}

/* X.25 CRC (CRC-16/MCRF4XX), initial value 0xFFFF */
static void crc_accumulate(uint8_t byte, uint16_t *crc)
{
	uint8_t tmp = byte ^ (uint8_t)(*crc & 0xFFU);

	tmp ^= (uint8_t)(tmp << 4);
	*crc = (uint16_t)((*crc >> 8) ^ ((uint16_t)tmp << 8) ^ ((uint16_t)tmp << 3) ^ ((uint16_t)tmp >> 4));
}

static uint16_t compute_crc(const uint8_t *buf, size_t len, uint8_t crc_extra)
{
	uint16_t crc = 0xFFFFU;

	for (size_t i = 0; i < len; i++) {
		crc_accumulate(buf[i], &crc);
	}
	crc_accumulate(crc_extra, &crc);
	return crc;
}

/* ── helpers to write little-endian fields into payload ── */
static void put_u8(uint8_t *p, uint8_t v)  { *p = v; }
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void put_i32(uint8_t *p, int32_t v)  { put_u32(p, (uint32_t)v); }
static void put_f32(uint8_t *p, float v)    { uint32_t tmp; memcpy(&tmp, &v, 4); put_u32(p, tmp); }

static uint8_t  get_u8(const uint8_t *p)  { return p[0]; }
static uint16_t get_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t get_u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static float    get_f32(const uint8_t *p) { uint32_t tmp = get_u32(p); float v; memcpy(&v, &tmp, 4); return v; }

/* ── Frame builder ── */
static int build_frame(uint8_t *buf, uint8_t seq,
		       uint8_t sys_id, uint8_t comp_id,
		       uint32_t msg_id, const uint8_t *payload, uint8_t plen)
{
	buf[0] = MAVLINK2_STX;
	buf[1] = plen;
	buf[2] = 0U; /* INC_FLAGS */
	buf[3] = 0U; /* CMP_FLAGS */
	buf[4] = seq;
	buf[5] = sys_id;
	buf[6] = comp_id;
	buf[7] = (uint8_t)(msg_id & 0xFFU);
	buf[8] = (uint8_t)((msg_id >> 8) & 0xFFU);
	buf[9] = (uint8_t)((msg_id >> 16) & 0xFFU);
	memcpy(&buf[10], payload, plen);

	uint16_t crc = compute_crc(&buf[1], (size_t)(MAVLINK2_HEADER_LEN - 1U) + plen,
				   get_crc_extra(msg_id));
	buf[10 + plen] = (uint8_t)(crc & 0xFFU);
	buf[11 + plen] = (uint8_t)(crc >> 8);
	return (int)(MAVLINK2_HEADER_LEN + plen + MAVLINK2_CHECKSUM_LEN);
}

/* ── Encoders ── */

int mavlink2_encode_heartbeat(uint8_t *buf, uint8_t seq,
			      uint8_t sys_id, uint8_t comp_id,
			      bool armed)
{
	uint8_t pl[9];

	put_u32(&pl[0], 0U); /* custom_mode */
	put_u8(&pl[4], MAV_TYPE_GIMBAL);
	put_u8(&pl[5], MAV_AUTOPILOT_INVALID);
	put_u8(&pl[6], armed ? MAV_MODE_FLAG_SAFETY_ARMED : 0U);
	put_u8(&pl[7], armed ? MAV_STATE_ACTIVE : MAV_STATE_STANDBY);
	put_u8(&pl[8], 3U); /* mavlink_version */
	return build_frame(buf, seq, sys_id, comp_id, MAVLINK_MSG_ID_HEARTBEAT, pl, 9U);
}

int mavlink2_encode_gimbal_attitude_status(uint8_t *buf, uint8_t seq,
					   uint8_t sys_id, uint8_t comp_id,
					   uint8_t target_sys, uint8_t target_comp,
					   const float q[4],
					   float wx, float wy, float wz,
					   uint32_t failure_flags, uint16_t flags)
{
	/*
	 * Wire order (by decreasing field size per MAVLink v2 spec):
	 * time_boot_ms(u32) q[4](f32×4) ang_vel_x/y/z(f32×3)
	 * failure_flags(u32) flags(u16) target_sys(u8) target_comp(u8)
	 * = 4+16+12+4+2+1+1 = 40 bytes
	 */
	uint8_t pl[40];
	int o = 0;

	put_u32(&pl[o], 0U); o += 4;               /* time_boot_ms – placeholder */
	put_f32(&pl[o], q[0]); o += 4;
	put_f32(&pl[o], q[1]); o += 4;
	put_f32(&pl[o], q[2]); o += 4;
	put_f32(&pl[o], q[3]); o += 4;
	put_f32(&pl[o], wx); o += 4;
	put_f32(&pl[o], wy); o += 4;
	put_f32(&pl[o], wz); o += 4;
	put_u32(&pl[o], failure_flags); o += 4;
	put_u16(&pl[o], flags); o += 2;
	put_u8(&pl[o], target_sys); o += 1;
	put_u8(&pl[o], target_comp); o += 1;
	return build_frame(buf, seq, sys_id, comp_id,
			   MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS, pl, (uint8_t)o);
}

int mavlink2_encode_command_ack(uint8_t *buf, uint8_t seq,
				uint8_t sys_id, uint8_t comp_id,
				uint16_t cmd, uint8_t result,
				uint8_t target_sys, uint8_t target_comp)
{
	/*
	 * Wire order: result_param2(i32) command(u16) result(u8)
	 * progress(u8) target_system(u8) target_component(u8) = 10 bytes
	 */
	uint8_t pl[10];
	int o = 0;

	put_i32(&pl[o], 0); o += 4;
	put_u16(&pl[o], cmd); o += 2;
	put_u8(&pl[o], result); o += 1;
	put_u8(&pl[o], 0U); o += 1;           /* progress */
	put_u8(&pl[o], target_sys); o += 1;
	put_u8(&pl[o], target_comp); o += 1;
	return build_frame(buf, seq, sys_id, comp_id,
			   MAVLINK_MSG_ID_COMMAND_ACK, pl, (uint8_t)o);
}

int mavlink2_encode_param_value(uint8_t *buf, uint8_t seq,
				uint8_t sys_id, uint8_t comp_id,
				const char *name, float value,
				uint16_t index, uint16_t count)
{
	/*
	 * Wire order: param_value(f32) param_count(u16) param_index(u16)
	 * param_id(char[16]) param_type(u8) = 4+2+2+16+1 = 25 bytes
	 */
	uint8_t pl[25];
	int o = 0;

	put_f32(&pl[o], value); o += 4;
	put_u16(&pl[o], count); o += 2;
	put_u16(&pl[o], index); o += 2;
	memset(&pl[o], 0, 16);
	strncpy((char *)&pl[o], name, 16);
	o += 16;
	put_u8(&pl[o], 9U); o += 1; /* PARAM_TYPE_REAL32 */
	return build_frame(buf, seq, sys_id, comp_id,
			   MAVLINK_MSG_ID_PARAM_VALUE, pl, (uint8_t)o);
}

int mavlink2_encode_statustext(uint8_t *buf, uint8_t seq,
			       uint8_t sys_id, uint8_t comp_id,
			       uint8_t severity, const char *text)
{
	/*
	 * Wire order: severity(u8) text(char[50]) id(u16) chunk_seq(u8)
	 * = 1+50+2+1 = 54 bytes, but MAVLink v1-compat trim to 51.
	 * We send 51 bytes: severity + text[50].
	 */
	uint8_t pl[51];

	put_u8(&pl[0], severity);
	memset(&pl[1], 0, 50);
	strncpy((char *)&pl[1], text, 50);
	return build_frame(buf, seq, sys_id, comp_id, 253UL, pl, 51U);
}

/* ── Parser ── */

int mavlink2_parse_byte(mavlink2_parser_t *p, uint8_t byte, mavlink2_frame_t *out)
{
	if (p->pos == 0U) {
		if (byte != MAVLINK2_STX) {
			return 0;
		}
	} else if (p->pos == 1U) {
		p->payload_len = byte;
	}

	p->buf[p->pos++] = byte;

	uint16_t expected_len = (uint16_t)(MAVLINK2_HEADER_LEN + p->payload_len + MAVLINK2_CHECKSUM_LEN);

	if (p->pos < expected_len) {
		return 0;
	}

	/* Verify CRC */
	uint32_t msg_id = (uint32_t)p->buf[7] | ((uint32_t)p->buf[8] << 8) | ((uint32_t)p->buf[9] << 16);
	uint16_t expected_crc = compute_crc(&p->buf[1],
					    (size_t)(MAVLINK2_HEADER_LEN - 1U) + p->payload_len,
					    get_crc_extra(msg_id));
	uint16_t rx_crc = (uint16_t)p->buf[10 + p->payload_len] |
			  ((uint16_t)p->buf[11 + p->payload_len] << 8);

	p->pos = 0U;

	if (rx_crc != expected_crc) {
		return 0;
	}

	out->msg_id      = msg_id;
	out->sys_id      = p->buf[5];
	out->comp_id     = p->buf[6];
	out->payload_len = p->payload_len;
	memcpy(out->payload, &p->buf[10], p->payload_len);
	return 1;
}

/* ── Decoders ── */

void mavlink2_decode_heartbeat(const mavlink2_frame_t *f,
			       uint8_t *base_mode, uint8_t *system_status)
{
	if (f->payload_len < 6U) {
		return;
	}
	*base_mode     = get_u8(&f->payload[6]);
	*system_status = get_u8(&f->payload[7]);
}

void mavlink2_decode_gimbal_set_attitude(const mavlink2_frame_t *f,
					 float q[4], float rates[3],
					 uint16_t *flags,
					 uint8_t *target_sys,
					 uint8_t *target_comp)
{
	if (f->payload_len < 32U) {
		return;
	}
	q[0]      = get_f32(&f->payload[0]);
	q[1]      = get_f32(&f->payload[4]);
	q[2]      = get_f32(&f->payload[8]);
	q[3]      = get_f32(&f->payload[12]);
	rates[0]  = get_f32(&f->payload[16]);
	rates[1]  = get_f32(&f->payload[20]);
	rates[2]  = get_f32(&f->payload[24]);
	*flags    = get_u16(&f->payload[28]);
	*target_sys  = get_u8(&f->payload[30]);
	*target_comp = get_u8(&f->payload[31]);
}

void mavlink2_decode_command_long(const mavlink2_frame_t *f,
				  uint16_t *cmd, float params[7],
				  uint8_t *target_sys, uint8_t *target_comp)
{
	if (f->payload_len < 33U) {
		return;
	}
	for (int i = 0; i < 7; i++) {
		params[i] = get_f32(&f->payload[i * 4]);
	}
	*cmd         = get_u16(&f->payload[28]);
	*target_sys  = get_u8(&f->payload[30]);
	*target_comp = get_u8(&f->payload[31]);
}

void mavlink2_decode_param_set(const mavlink2_frame_t *f,
			       char name[17], float *value,
			       uint8_t *target_sys, uint8_t *target_comp)
{
	if (f->payload_len < 23U) {
		return;
	}
	*value       = get_f32(&f->payload[0]);
	*target_sys  = get_u8(&f->payload[4]);
	*target_comp = get_u8(&f->payload[5]);
	memcpy(name, &f->payload[6], 16);
	name[16] = '\0';
}
