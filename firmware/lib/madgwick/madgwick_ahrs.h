#pragma once

typedef struct {
	float q[4];   /* quaternion [w, x, y, z] */
	float beta;
	float dt;
} madgwick_t;

void madgwick_init(madgwick_t *m, float beta, float dt);

/* gyro in deg/s, accel in m/s² */
void madgwick_update(madgwick_t *m,
		     float gx, float gy, float gz,
		     float ax, float ay, float az);

/* output angles in degrees */
void madgwick_get_euler(const madgwick_t *m,
			float *roll, float *pitch, float *yaw);
