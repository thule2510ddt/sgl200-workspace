#pragma once

#include <stdint.h>
#include <zephyr/kernel.h>

struct imu_sample_t {
	int64_t timestamp_us;
	float accel_mps2[3];
	float gyro_dps[3];
	float temp_c;
};

/*
 * data_ready_sem is given from the INT GPIO ISR (PB0) each time the
 * ICM-42688-P asserts its data-ready interrupt (1 kHz).
 */
int icm42688_init(struct k_sem *data_ready_sem);
int icm42688_read_sample(struct imu_sample_t *out);
