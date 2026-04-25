#include <math.h>
#include "madgwick_ahrs.h"

#define DEG_TO_RAD 0.017453292519943295f
#define RAD_TO_DEG 57.29577951308232f

void madgwick_init(madgwick_t *m, float beta, float dt)
{
	m->q[0] = 1.0f;
	m->q[1] = 0.0f;
	m->q[2] = 0.0f;
	m->q[3] = 0.0f;
	m->beta = beta;
	m->dt = dt;
}

void madgwick_update(madgwick_t *m,
		     float gx, float gy, float gz,
		     float ax, float ay, float az)
{
	float q0 = m->q[0], q1 = m->q[1], q2 = m->q[2], q3 = m->q[3];

	gx *= DEG_TO_RAD;
	gy *= DEG_TO_RAD;
	gz *= DEG_TO_RAD;

	float norm = sqrtf(ax * ax + ay * ay + az * az);

	if (norm < 1e-6f) {
		goto gyro_only;
	}
	norm = 1.0f / norm;
	ax *= norm;
	ay *= norm;
	az *= norm;

	{
		float _2q0 = 2.0f * q0;
		float _2q1 = 2.0f * q1;
		float _2q2 = 2.0f * q2;
		float _2q3 = 2.0f * q3;
		float _4q0 = 4.0f * q0;
		float _4q1 = 4.0f * q1;
		float _4q2 = 4.0f * q2;
		float _8q1 = 8.0f * q1;
		float _8q2 = 8.0f * q2;
		float q0q0 = q0 * q0;
		float q1q1 = q1 * q1;
		float q2q2 = q2 * q2;
		float q3q3 = q3 * q3;

		float s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
		float s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1
			   - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
		float s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3
			   - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
		float s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

		float snorm = sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);

		if (snorm > 1e-6f) {
			snorm = 1.0f / snorm;
			s0 *= snorm;
			s1 *= snorm;
			s2 *= snorm;
			s3 *= snorm;
		}

		float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - m->beta * s0;
		float qDot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy) - m->beta * s1;
		float qDot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx) - m->beta * s2;
		float qDot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx) - m->beta * s3;

		q0 += qDot0 * m->dt;
		q1 += qDot1 * m->dt;
		q2 += qDot2 * m->dt;
		q3 += qDot3 * m->dt;

		goto normalize;
	}

gyro_only:
	{
		float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
		float qDot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
		float qDot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
		float qDot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

		q0 += qDot0 * m->dt;
		q1 += qDot1 * m->dt;
		q2 += qDot2 * m->dt;
		q3 += qDot3 * m->dt;
	}

normalize:
	{
		float qnorm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);

		if (qnorm > 1e-6f) {
			qnorm = 1.0f / qnorm;
			m->q[0] = q0 * qnorm;
			m->q[1] = q1 * qnorm;
			m->q[2] = q2 * qnorm;
			m->q[3] = q3 * qnorm;
		}
	}
}

void madgwick_get_euler(const madgwick_t *m, float *roll, float *pitch, float *yaw)
{
	float q0 = m->q[0], q1 = m->q[1], q2 = m->q[2], q3 = m->q[3];

	*roll  = atan2f(2.0f * (q0 * q1 + q2 * q3),
			1.0f - 2.0f * (q1 * q1 + q2 * q2)) * RAD_TO_DEG;
	*pitch = asinf(2.0f * (q0 * q2 - q3 * q1)) * RAD_TO_DEG;
	*yaw   = atan2f(2.0f * (q0 * q3 + q1 * q2),
			1.0f - 2.0f * (q2 * q2 + q3 * q3)) * RAD_TO_DEG;
}
