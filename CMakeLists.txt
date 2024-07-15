#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Add the driver directory as a Zephyr module
list(APPEND EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/ti_ads1299_driver
     ${CMAKE_CURRENT_SOURCE_DIR}/bosch_bmi270_driver)

set(BOARD_ROOT ${CMAKE_SOURCE_DIR})
set(BOARD eeg_nrf5340_cpuapp)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(empty_app_core)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
