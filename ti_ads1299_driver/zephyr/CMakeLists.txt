# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TI_ADS1299_DRIVER)
  # Add custom_bme280_driver_spi.h to the set of global include paths.
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(ti_ads1299_driver_spi.c)
endif()
