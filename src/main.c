/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

// #include "ads1299_driver_spi.h"

LOG_MODULE_REGISTER(EEG_MAIN, LOG_LEVEL_INF);

const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(ads1299));

int main(void)
{
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	// int err = device_is_ready(spi_dev);
	// if (!err) {
	// 	LOG_INF("Error: SPI device is not ready, err: %d", err);
	// 	return 0;
	// }

	return 0;
}
