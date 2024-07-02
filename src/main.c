/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(EEG_MAIN, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	return 0;
}
