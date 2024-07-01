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

#define SPIOP SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "TI ADS1299 driver enabled without any devices"
#endif

/* Data structure to store BME280 data */
struct ads1299_data {
	uint8_t chip_id;
} adsdata;

struct ti_ads1299_config {
	struct spi_dt_spec spi;
};

static int init(const struct device *dev)
{
	int err;

	const struct ti_ads1299_config *bme280_config = dev->config;
	err = spi_is_ready_dt(&bme280_config->spi);
	if (!err) {
		printk("Error: SPI device is not ready, err: %d", err);
		return 0;
	}

	return 0;
}

static void bme280_print(const struct device *dev)
{
	printk("Print all characteristics of BME280 sensor\n");
}

static const struct ti_ads1299_driver_api ti_ads1299_api_funcs = {
	.print = bme280_print,
};

/* Initializes a struct bme280_config for an instance on a SPI bus. */
#define ADS1299_CONFIG_SPI(inst)                             \
	{                                                    \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPIOP, 0), \
	}

/* STEP 5.1 - Define a device driver instance */
#define CUSTOM_BME280_DEFINE(inst)                                       \
	static struct ads1299_data ads1299_data_##inst;                  \
	static const struct ti_ads1299_config ti_ads1299_config_##inst = \
		ADS1299_CONFIG_SPI(inst);                                \
	DEVICE_DT_INST_DEFINE(inst, init, NULL, &ads1299_data_##inst,    \
			      &ti_ads1299_config_##inst, POST_KERNEL,    \
			      CONFIG_DRIVER_INIT_PRIORITY,               \
			      &ti_ads1299_api_funcs);

/* STEP 5.2 - Create the struct device for every status "okay" node in the devicetree */
DT_INST_FOREACH_STATUS_OKAY(CUSTOM_BME280_DEFINE)
