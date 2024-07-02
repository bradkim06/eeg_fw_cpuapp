/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ti_ads1299_driver_spi.h"
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <ncs_version.h>
#if NCS_VERSION_NUMBER >= 0x20600
#include <zephyr/internal/syscall_handler.h>
#else
#include <zephyr/syscall_handler.h>
#endif

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define DELAY_REG 2
#define DELAY_PARAM 10

// command
#define WAKEUP 0x02
#define STANDBY 0x04
#define RESET 0x06
#define START 0x08
#define STOP 0x0A

#define RDATAC 0x10
#define SDATAC 0x11
#define RDATA 0x12

// register
#define ID_REG 0x00
#define CONFIG1_REG 0x01
#define CONFIG2_REG 0x02
#define CONFIG3_REG 0x03
#define LOFF_REG 0x04
#define CH1SET_REG 0x05
#define CH2SET_REG 0x06
#define CH3SET_REG 0x07
#define CH4SET_REG 0x08
#define CH5SET_REG 0x09
#define CH6SET_REG 0x0A
#define CH7SET_REG 0x0B
#define CH8SET_REG 0x0C
#define BIAS_SENSP_REG 0x0D
#define BIAS_SENSN_REG 0x0E
#define LOFF_SENSP_REG 0x0F
#define LOFF_SENSN_REG 0x10
#define LOFF_FLIP_REG 0x11
#define LOFF_STATP_REG 0x12
#define LOFF_STATN_REG 0x13
#define GPIO_REG 0x14
#define MISC1_REG 0x15
#define MISC2_REG 0x16
#define CONFIG4_REG 0x17

#define ADS1299_SPI_OPERATION \
	(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA)
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "TI ADS1299 driver enabled without any devices"
#endif

/* Data structure to store ADS1299 data */
struct ads1299_data {
	uint8_t chip_id;
} adsdata;

struct ti_ads1299_config {
	struct spi_dt_spec spi;
};

// ADS1299 레지스터 쓰기 함수
static int write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	uint8_t tx_data[] = {
		0x40 | reg, 0x00, value
	}; // 첫 바이트: 0x40 (쓰기 명령) | 레지스터 주소
	struct spi_buf tx_buf = { .buf = tx_data, .len = sizeof(tx_data) };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	const struct ti_ads1299_config *ads1299_config = dev->config;
	if (spi_write_dt(&ads1299_config->spi, &tx_bufs) != 0) {
		printk("SPI write failed");
		return -EIO;
	}

	return 0;
}

// ADS1299 레지스터 읽기 함수
static int read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	uint8_t tx_data[] = {
		0x20 | reg, 0x00
	}; // 첫 바이트: 0x20 (읽기 명령) | 레지스터 주소
	uint8_t rx_data[3];
	struct spi_buf tx_buf = { .buf = tx_data, .len = sizeof(tx_data) };
	struct spi_buf rx_buf = { .buf = rx_data, .len = sizeof(rx_data) };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };
	struct spi_buf_set rx_bufs = { .buffers = &rx_buf, .count = 1 };

	const struct ti_ads1299_config *ads1299_config = dev->config;
	if (spi_transceive_dt(&ads1299_config->spi, &tx_bufs, &rx_bufs) != 0) {
		printk("SPI transceive failed");
		return -EIO;
	}
	k_msleep(DELAY_REG);

	//TODO Bit1 개가 쉬프트되는 문제
	*value = rx_data[2]; // 세 번째 바이트가 실제 데이터
	return 0;
}

static int send_command(const struct device *dev, uint8_t command)
{
	struct spi_buf tx_buf = { .buf = &command, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	const struct ti_ads1299_config *ads1299_config = dev->config;
	if (spi_write_dt(&ads1299_config->spi, &tx_bufs) != 0) {
		printk("SPI write SDATAC command failed");
		return -EIO;
	}

	// SDATAC 명령 후 잠시 대기
	k_msleep(DELAY_REG);

	return 0;
}

static int init(const struct device *dev)
{
	int err;

	const struct ti_ads1299_config *ads1299_config = dev->config;
	err = spi_is_ready_dt(&ads1299_config->spi);
	if (!err) {
		printk("Error: SPI device is not ready, err: %d\n", err);
		return err;
	}

	// See Detail ADS1299 Data Sheet(Figure 67. Initial Flow at Power-Up)
	// 1. Send SDATAC command
	err = send_command(dev, SDATAC);
	if (err != 0) {
		printk("Failed to send SDATAC command, err: %d\n", err);
		return err;
	}

	// 2. Enable internal reference buffer, BIASREF_INT=1
	err = write_reg(dev, CONFIG3_REG, 0xE8);
	if (err != 0) {
		printk("Failed to set CONFIG3 register\n");
		return err;
	}

	// 3. Wait for reference buffer to settle
	k_sleep(K_MSEC(150)); // > 150 ms for internal reference buffer

	// 4. Configure device
	// Set CONFIG1: DR = fMOD/4096
	err = write_reg(dev, CONFIG1_REG, 0x96);
	if (err != 0) {
		printk("Failed to set CONFIG1 register\n");
		return err;
	}

	// Set CONFIG2: Test signal
	err = write_reg(dev, CONFIG2_REG, 0xC0);
	if (err != 0) {
		printk("Failed to set CONFIG2 register\n");
		return err;
	}

	// Set All Channels to Input Short
	for (int i = 0; i < 4; i++) {
		err = write_reg(dev, CH1SET_REG + i, 0x01);
		if (err != 0) {
			printk("Failed to set CH%dSET register\n", i + 1);
			return err;
		}
	}

	printk("---------- ADS1299 initial configuration completed successfully ----------\n");

	return 0;
}

static void ads1299_config_print(const struct device *dev)
{
	printk("---------- Print all characteristics of ADS1299 sensor ----------\n");

	printk("ADS1299 Settings:\n");

	uint8_t reg_value;
	// ID 레지스터 읽기
	int err = read_reg(dev, ID_REG, &reg_value);
	if (err == 0) {
		printk("ID (0x00): 0x%02X\n", reg_value);
		printk("  - Device: ADS%d\n",
		       (reg_value & 0x07) == 0 ? 1299 : 1298);
	}
	k_msleep(DELAY_PARAM);

	// CONFIG1 레지스터 읽기
	err = read_reg(dev, CONFIG1_REG, &reg_value);
	if (err == 0) {
		printk("CONFIG1 (0x01): 0x%X\n", reg_value);
		printk("  - Daisy-chain mode: %s\n",
		       (reg_value & 0x40) ? "Enabled" : "Disabled");
		printk("  - CLK output: %s\n",
		       (reg_value & 0x20) ? "Enabled" : "Disabled");
		printk("  - Data rate: %d SPS\n", 16000 >> (reg_value & 0x07));
	}
	k_msleep(DELAY_PARAM);

	// CONFIG2 레지스터 읽기
	err = read_reg(dev, CONFIG2_REG, &reg_value);
	if (err == 0) {
		printk("CONFIG2 (0x02): 0x%X\n", reg_value);
		printk("  - Test signal: %s\n",
		       (reg_value & 0x10) ? "Enabled" : "Disabled");
	}
	k_msleep(DELAY_PARAM);

	// CONFIG3 레지스터 읽기
	err = read_reg(dev, CONFIG3_REG, &reg_value);
	if (err == 0) {
		printk("CONFIG3 (0x03): 0x%02X\n", reg_value);
		printk("  - Internal reference buffer: %s\n",
		       (reg_value & 0x80) ? "Enabled" : "Disabled");
		printk("  - Bias measurement: %s\n",
		       (reg_value & 0x40) ? "Enabled" : "Disabled");
		printk("  - Bias reference: %s\n",
		       (reg_value & 0x20) ? "Internal" : "External");
		printk("  - Bias buffer: %s\n",
		       (reg_value & 0x10) ? "Enabled" : "Disabled");
	}
	k_msleep(DELAY_PARAM);

	// 채널 설정 읽기
	for (int i = 0; i < 8; i++) {
		err = read_reg(dev, CH1SET_REG + i, &reg_value);
		if (err == 0) {
			printk("CH%dSET (0x%02X): 0x%02X\n", i + 1,
			       CH1SET_REG + i, reg_value);
			printk("  - PGA gain: %dV/V\n",
			       1 << (reg_value & 0x07));
			printk("  - Channel power: %s\n",
			       (reg_value & 0x80) ? "Normal" : "Power-down");
			printk("  - SRB2 connection: %s\n",
			       (reg_value & 0x08) ? "Closed" : "Open");
		}
		k_msleep(DELAY_PARAM);
	}

	// BIAS_SENSP 레지스터 읽기
	err = read_reg(dev, BIAS_SENSP_REG, &reg_value);
	if (err == 0) {
		printk("BIAS_SENSP (0x0D): 0x%02X\n", reg_value);
		for (int i = 0; i < 8; i++) {
			printk("  - Channel %d bias: %s\n", i + 1,
			       (reg_value & (1 << i)) ? "Enabled" : "Disabled");
			k_msleep(DELAY_PARAM);
		}
	}

	// BIAS_SENSN 레지스터 읽기
	err = read_reg(dev, BIAS_SENSN_REG, &reg_value);
	if (err == 0) {
		printk("BIAS_SENSN (0x0E): 0x%02X\n", reg_value);
		for (int i = 0; i < 8; i++) {
			printk("  - Channel %d bias: %s\n", i + 1,
			       (reg_value & (1 << i)) ? "Enabled" : "Disabled");
			k_msleep(DELAY_PARAM);
		}
	}

	// LOFF_SENSP 레지스터 읽기
	err = read_reg(dev, LOFF_SENSP_REG, &reg_value);
	if (err == 0) {
		printk("LOFF_SENSP (0x0F): 0x%02X\n", reg_value);
		for (int i = 0; i < 8; i++) {
			printk("  - Channel %d lead-off detection P: %s\n",
			       i + 1,
			       (reg_value & (1 << i)) ? "Enabled" : "Disabled");
			k_msleep(DELAY_PARAM);
		}
	}

	// LOFF_SENSN 레지스터 읽기
	err = read_reg(dev, LOFF_SENSN_REG, &reg_value);
	if (err == 0) {
		printk("LOFF_SENSN (0x10): 0x%02X\n", reg_value);
		for (int i = 0; i < 8; i++) {
			printk("  - Channel %d lead-off detection N: %s\n",
			       i + 1,
			       (reg_value & (1 << i)) ? "Enabled" : "Disabled");
			k_msleep(DELAY_PARAM);
		}
	}

	// GPIO 레지스터 읽기
	err = read_reg(dev, GPIO_REG, &reg_value);
	if (err == 0) {
		printk("GPIO (0x14): 0x%02X\n", reg_value);
		printk("  - GPIO1 direction: %s\n",
		       (reg_value & 0x01) ? "Output" : "Input");
		printk("  - GPIO2 direction: %s\n",
		       (reg_value & 0x02) ? "Output" : "Input");
		printk("  - GPIO3 direction: %s\n",
		       (reg_value & 0x04) ? "Output" : "Input");
		printk("  - GPIO4 direction: %s\n",
		       (reg_value & 0x08) ? "Output" : "Input");
	}
	k_msleep(DELAY_PARAM);

	// CONFIG4 레지스터 읽기
	err = read_reg(dev, CONFIG4_REG, &reg_value);
	if (err == 0) {
		printk("CONFIG4 (0x17): 0x%02X\n", reg_value);
		printk("  - Lead-off comparator threshold: %d\n",
		       (reg_value & 0x03));
		printk("  - Lead-off current magnitude: %d\n",
		       ((reg_value >> 2) & 0x03));
	}
}

static int ads1299_read_reg(const struct device *dev, uint8_t reg,
			    uint8_t *value)
{
	printk("Read to a given register of ADS1299\n");

	return read_reg(dev, reg, value);
}

static int ads1299_write_reg(const struct device *dev, uint8_t reg,
			     uint8_t value)
{
	printk("Write to a given register of ADS1299\n");

	return write_reg(dev, reg, value);
}

static int ads1299_command(const struct device *dev, uint8_t cmd)
{
	printk("Send command to ADS1299\n");

	return send_command(dev, cmd);
}

static int ads1299_read_data(const struct device *dev, uint8_t *data,
			     size_t len)
{
	struct spi_buf rx_buf = { .buf = data, .len = len };
	struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	const struct ti_ads1299_config *ads1299_config = dev->config;

	return spi_read_dt(&ads1299_config->spi, &rx);
}

static const struct ti_ads1299_driver_api ti_ads1299_api_funcs = {
	.config = ads1299_config_print,
	.read_reg = ads1299_read_reg,
	.write_reg = ads1299_write_reg,
	.command = ads1299_command,
	.read_data = ads1299_read_data,
};

/* Initializes a struct ads1299_config for an instance on a SPI bus. */
#define ADS1299_CONFIG_SPI(inst)                                             \
	{                                                                    \
		.spi = SPI_DT_SPEC_INST_GET(inst, ADS1299_SPI_OPERATION, 0), \
	}

/* STEP 5.1 - Define a device driver instance */
#define TI_ADS1299_DEFINE(inst)                                          \
	static struct ads1299_data ads1299_data_##inst;                  \
	static const struct ti_ads1299_config ti_ads1299_config_##inst = \
		ADS1299_CONFIG_SPI(inst);                                \
	DEVICE_DT_INST_DEFINE(inst, init, NULL, &ads1299_data_##inst,    \
			      &ti_ads1299_config_##inst, POST_KERNEL,    \
			      CONFIG_DRIVER_INIT_PRIORITY,               \
			      &ti_ads1299_api_funcs);

/* STEP 5.2 - Create the struct device for every status "okay" node in the devicetree */
DT_INST_FOREACH_STATUS_OKAY(TI_ADS1299_DEFINE)
