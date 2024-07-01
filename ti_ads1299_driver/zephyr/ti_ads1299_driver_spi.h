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

/* STEP 4 - Understand four parts of this header file in detail */

/* STEP 4.1 - Typedef declaration of the function pointers */
typedef void (*ti_ads1299_api_print_t)(const struct device *dev);

/* STEP 4.2 - Define a struct to have a member for each typedef you defined in Part 1 */
struct ti_ads1299_driver_api {
	ti_ads1299_api_print_t print;
};

/* STEP 4.3 - Implement the API to be exposed to the application with type and arguments matching the typedef */
__syscall void ti_ads1299_print(const struct device *dev);

/* STEP 4.4 - Implement the Z_impl_* translation function to call the device driver API for this feature */
static inline void z_impl_ti_ads1299_print(const struct device *dev)
{
	const struct ti_ads1299_driver_api *api = dev->api;

	__ASSERT(api->print, "Callback pointer should not be NULL");

	api->print(dev);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/ti_ads1299_driver_spi.h>

#endif /* __TI_ADS1299_DRIVER_H__ */
