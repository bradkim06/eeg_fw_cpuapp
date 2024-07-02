/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __TI_ADS1299_DRIVER_H__
#define __TI_ADS1299_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

#define DT_DRV_COMPAT ti_ads1299

/* Typedef declaration of the function pointers */
typedef void (*ti_ads1299_api_config_t)(const struct device *dev);
typedef int (*ti_ads1299_api_read_reg_t)(const struct device *dev, uint8_t reg,
					 uint8_t *value);
typedef int (*ti_ads1299_api_write_reg_t)(const struct device *dev, uint8_t reg,
					  uint8_t value);
typedef int (*ti_ads1299_api_command_t)(const struct device *dev, uint8_t cmd);
typedef int (*ti_ads1299_api_read_data_t)(const struct device *dev,
					  uint8_t *data, size_t len);

/* Define a struct to have a member for each typedef you defined in Part 1 */
struct ti_ads1299_driver_api {
	ti_ads1299_api_config_t config;
	ti_ads1299_api_read_reg_t read_reg;
	ti_ads1299_api_write_reg_t write_reg;
	ti_ads1299_api_command_t command;
	ti_ads1299_api_read_data_t read_data;
};

/* Implement the API to be exposed to the application with type and arguments matching the typedef */
__syscall void ti_ads1299_config(const struct device *dev);
/* Implement the Z_impl_* translation function to call the device driver API for this feature */
static inline void z_impl_ti_ads1299_config(const struct device *dev)
{
	const struct ti_ads1299_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");

	api->config(dev);
}

__syscall int ti_ads1299_read_reg(const struct device *dev, uint8_t reg,
				  uint8_t *value);
static inline int z_impl_ti_ads1299_read_reg(const struct device *dev,
					     uint8_t reg, uint8_t *value)
{
	const struct ti_ads1299_driver_api *api = dev->api;

	__ASSERT(api->read_reg, "Callback pointer should not be NULL");

	return api->read_reg(dev, reg, value);
}

__syscall int ti_ads1299_write_reg(const struct device *dev, uint8_t reg,
				   uint8_t value);
static inline int z_impl_ti_ads1299_write_reg(const struct device *dev,
					      uint8_t reg, uint8_t value)
{
	const struct ti_ads1299_driver_api *api = dev->api;

	__ASSERT(api->write_reg, "Callback pointer should not be NULL");

	return api->write_reg(dev, reg, value);
}

__syscall int ti_ads1299_command(const struct device *dev, uint8_t cmd);
static inline int z_impl_ti_ads1299_command(const struct device *dev,
					    uint8_t cmd)
{
	const struct ti_ads1299_driver_api *api = dev->api;

	__ASSERT(api->command, "Callback pointer should not be NULL");

	return api->command(dev, cmd);
}

__syscall int ti_ads1299_read_data(const struct device *dev, uint8_t *data,
				   size_t len);
static inline int z_impl_ti_ads1299_read_data(const struct device *dev,
					      uint8_t *data, size_t len)
{
	const struct ti_ads1299_driver_api *api = dev->api;

	__ASSERT(api->read_data, "Callback pointer should not be NULL");

	return api->read_data(dev, data, len);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/ti_ads1299_driver_spi.h>

#endif /* __TI_ADS1299_DRIVER_H__ */
