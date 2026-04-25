#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "icm42688.h"

LOG_MODULE_REGISTER(icm42688, LOG_LEVEL_INF);

/* ── Register map ── */
#define REG_DEVICE_CONFIG   0x11U
#define REG_PWR_MGMT0       0x4EU
#define REG_GYRO_CONFIG0    0x4FU
#define REG_ACCEL_CONFIG0   0x50U
#define REG_INT_CONFIG      0x14U
#define REG_INT_CONFIG1     0x64U
#define REG_INT_SOURCE0     0x65U
#define REG_WHO_AM_I        0x75U
#define REG_TEMP_DATA1      0x1DU

/* ── PWR_MGMT0 ── */
#define PWR_ACCEL_LN        0x03U
#define PWR_GYRO_LN         0x0CU

/* ── GYRO_CONFIG0 / ACCEL_CONFIG0 ── */
/* FS_SEL[7:5] | ODR[3:0] — ±2000dps / ±16g, 1kHz */
#define GYRO_CFG_2000DPS_1KHZ   0x06U
#define ACCEL_CFG_16G_1KHZ      0x06U

/* ── Sensitivity (LN mode, full scale) ── */
#define GYRO_SENS_2000DPS  (1.0f / 16.4f)    /* deg/s per LSB */
#define ACCEL_SENS_16G     (1.0f / 2048.0f)  /* g per LSB */
#define GRAVITY_MPS2       9.80665f

/* ── SPI spec from DTS ── */
#define ICM_NODE DT_NODELABEL(icm42688)
static const struct spi_dt_spec spi = SPI_DT_SPEC_GET(
	ICM_NODE,
	SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER |
	SPI_MODE_CPOL | SPI_MODE_CPHA,
	2U);

/* ── INT GPIO from DTS ── */
static const struct gpio_dt_spec int_gpio = GPIO_DT_SPEC_GET(ICM_NODE, int_gpios);
static struct gpio_callback int_cb_data;
static struct k_sem *g_data_ready;

static void int_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	k_sem_give(g_data_ready);
}

/* ── Low-level SPI helpers ── */
static int reg_write(uint8_t reg, uint8_t val)
{
	uint8_t tx[2] = { reg & 0x7FU, val };
	struct spi_buf tx_buf = { .buf = tx, .len = 2U };
	struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1U };

	return spi_transceive_dt(&spi, &tx_set, NULL);
}

static int reg_read(uint8_t reg, uint8_t *buf, size_t len)
{
	uint8_t addr = reg | 0x80U;
	struct spi_buf tx_buf[2] = {
		{ .buf = &addr, .len = 1U },
		{ .buf = NULL,  .len = len },
	};
	struct spi_buf rx_buf[2] = {
		{ .buf = NULL, .len = 1U },
		{ .buf = buf,  .len = len },
	};
	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 2U };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 2U };

	return spi_transceive_dt(&spi, &tx_set, &rx_set);
}

/* ── Public API ── */

int icm42688_init(struct k_sem *data_ready_sem)
{
	int ret;
	uint8_t who_am_i;

	g_data_ready = data_ready_sem;

	if (!spi_is_ready_dt(&spi)) {
		LOG_ERR("SPI not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&int_gpio)) {
		LOG_ERR("INT GPIO not ready");
		return -ENODEV;
	}

	/* Soft reset */
	ret = reg_write(REG_DEVICE_CONFIG, 0x01U);
	if (ret) {
		return ret;
	}
	k_sleep(K_MSEC(2));

	/* Verify device identity */
	ret = reg_read(REG_WHO_AM_I, &who_am_i, 1U);
	if (ret) {
		return ret;
	}
	if (who_am_i != 0x47U) {
		LOG_ERR("WHO_AM_I = 0x%02X, expected 0x47", who_am_i);
		return -ENODEV;
	}

	/* Power up accel + gyro in low-noise mode */
	ret = reg_write(REG_PWR_MGMT0, PWR_ACCEL_LN | PWR_GYRO_LN);
	if (ret) {
		return ret;
	}
	k_sleep(K_MSEC(1));

	/* ±2000dps, 1kHz */
	ret = reg_write(REG_GYRO_CONFIG0, GYRO_CFG_2000DPS_1KHZ);
	if (ret) {
		return ret;
	}

	/* ±16g, 1kHz */
	ret = reg_write(REG_ACCEL_CONFIG0, ACCEL_CFG_16G_1KHZ);
	if (ret) {
		return ret;
	}

	/* INT1: push-pull, active-high */
	ret = reg_write(REG_INT_CONFIG, 0x03U);
	if (ret) {
		return ret;
	}

	/* Disable deassert to keep INT1 pulse clean */
	ret = reg_write(REG_INT_CONFIG1, 0x00U);
	if (ret) {
		return ret;
	}

	/* Enable data-ready on INT1 */
	ret = reg_write(REG_INT_SOURCE0, 0x08U);
	if (ret) {
		return ret;
	}

	/* Configure GPIO interrupt (active-high rising edge) */
	ret = gpio_pin_configure_dt(&int_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&int_gpio, GPIO_INT_EDGE_RISING);
	if (ret) {
		return ret;
	}
	gpio_init_callback(&int_cb_data, int_isr, BIT(int_gpio.pin));
	ret = gpio_add_callback(int_gpio.port, &int_cb_data);
	if (ret) {
		return ret;
	}

	LOG_INF("ICM-42688-P ready (WHO_AM_I=0x%02X)", who_am_i);
	return 0;
}

int icm42688_read_sample(struct imu_sample_t *out)
{
	/*
	 * Burst-read 14 bytes from REG_TEMP_DATA1 (0x1D):
	 * [0-1] TEMP, [2-7] ACCEL XYZ, [8-13] GYRO XYZ  (each big-endian i16)
	 */
	uint8_t raw[14];
	int ret = reg_read(REG_TEMP_DATA1, raw, sizeof(raw));

	if (ret) {
		return ret;
	}

	int16_t temp_raw   = (int16_t)((uint16_t)raw[0]  << 8 | raw[1]);
	int16_t accel_x    = (int16_t)((uint16_t)raw[2]  << 8 | raw[3]);
	int16_t accel_y    = (int16_t)((uint16_t)raw[4]  << 8 | raw[5]);
	int16_t accel_z    = (int16_t)((uint16_t)raw[6]  << 8 | raw[7]);
	int16_t gyro_x     = (int16_t)((uint16_t)raw[8]  << 8 | raw[9]);
	int16_t gyro_y     = (int16_t)((uint16_t)raw[10] << 8 | raw[11]);
	int16_t gyro_z     = (int16_t)((uint16_t)raw[12] << 8 | raw[13]);

	out->timestamp_us   = k_ticks_to_us_floor64(k_uptime_ticks());
	out->temp_c         = (float)temp_raw / 132.48f + 25.0f;
	out->accel_mps2[0]  = (float)accel_x * ACCEL_SENS_16G * GRAVITY_MPS2;
	out->accel_mps2[1]  = (float)accel_y * ACCEL_SENS_16G * GRAVITY_MPS2;
	out->accel_mps2[2]  = (float)accel_z * ACCEL_SENS_16G * GRAVITY_MPS2;
	out->gyro_dps[0]    = (float)gyro_x  * GYRO_SENS_2000DPS;
	out->gyro_dps[1]    = (float)gyro_y  * GYRO_SENS_2000DPS;
	out->gyro_dps[2]    = (float)gyro_z  * GYRO_SENS_2000DPS;
	return 0;
}
