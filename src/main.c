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

#include "ti_ads1299_driver_spi.h"

LOG_MODULE_REGISTER(EEG_MAIN, LOG_LEVEL_INF);

#define ADS1299_NODE DT_NODELABEL(ads1299)

const struct device *ads1299_spi_dev = DEVICE_DT_GET(DT_NODELABEL(ads1299));

static K_SEM_DEFINE(drdy_sem, 0, 1);
static struct gpio_callback drdy_cb_data;

// DRDY 인터럽트 핸들러
static void drdy_handler(const struct device *dev, struct gpio_callback *cb,
			 uint32_t pins)
{
	printk("drdy int\n");
	k_sem_give(&drdy_sem);
}

static int ads1299_init(void)
{
	const struct device *drdy_gpio_dev;
	int ret;

	// DRDY GPIO 디바이스 가져오기
	drdy_gpio_dev = DEVICE_DT_GET(DT_GPIO_CTLR(ADS1299_NODE, drdy_gpios));
	if (!device_is_ready(drdy_gpio_dev)) {
		printk("DRDY GPIO device not ready\n");
		return -ENODEV;
	}

	// DRDY 핀을 입력으로 구성
	ret = gpio_pin_configure(
		drdy_gpio_dev, DT_GPIO_PIN(ADS1299_NODE, drdy_gpios),
		GPIO_INPUT | DT_GPIO_FLAGS(ADS1299_NODE, drdy_gpios));
	if (ret != 0) {
		printk("Error configuring DRDY pin: %d\n", ret);
		return ret;
	}

	// DRDY 인터럽트 설정
	ret = gpio_pin_interrupt_configure(
		drdy_gpio_dev, DT_GPIO_PIN(ADS1299_NODE, drdy_gpios),
		GPIO_INT_EDGE_FALLING);
	if (ret != 0) {
		printk("Error configuring DRDY interrupt: %d\n", ret);
		return ret;
	}

	// 콜백 초기화 및 추가
	gpio_init_callback(&drdy_cb_data, drdy_handler,
			   BIT(DT_GPIO_PIN(ADS1299_NODE, drdy_gpios)));
	ret = gpio_add_callback(drdy_gpio_dev, &drdy_cb_data);
	if (ret != 0) {
		printk("Error adding DRDY callback: %d\n", ret);
		return ret;
	}

	printk("ADS1299 DRDY interrupt initialized\n");
	return 0;
}

int main(void)
{
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	int err = device_is_ready(ads1299_spi_dev);
	if (!err) {
		LOG_INF("Error: SPI device is not ready, err: %d", err);
		return 0;
	}

	ads1299_init();

	ti_ads1299_config(ads1299_spi_dev);

	uint8_t data[15]; // 3 bytes status + 12 bytes data (4 channels * 3 bytes per channel)
	while (1) {
#define START 0x08
		ti_ads1299_command(ads1299_spi_dev, START);
		k_sem_take(&drdy_sem, K_FOREVER);
		printk("data read\n");

		if (ti_ads1299_read_data(ads1299_spi_dev, data, sizeof(data)) ==
		    0) {
			printk("Data read: ");
			for (int i = 0; i < sizeof(data); i++) {
				printk("%02X ", data[i]);
			}
			printk("\n");
		} else {
			printk("Error reading data from ADS1299\n");
		}

		k_msleep(10000);
	}

	return 0;
}
