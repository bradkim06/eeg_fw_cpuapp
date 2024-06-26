#ifndef __APP_BT_H__
#define __APP_BT_H__

#include <stdbool.h>

#include "hhs_util.h"

/** @brief LBS Service UUID. */
#define BT_UUID_HHS_VAL \
	BT_UUID_128_ENCODE(0x0000FFF0, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB)
/** @brief Notify Characteristic UUID. */
#define BT_UUID_HHS_NOTI_VAL \
	BT_UUID_128_ENCODE(0x0000FFF1, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB)
/** @brief Write Characteristic UUID. */
#define BT_UUID_HHS_WRITE_VAL \
	BT_UUID_128_ENCODE(0x0000FFF2, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB)

#define BT_UUID_HHS BT_UUID_DECLARE_128(BT_UUID_HHS_VAL)
#define BT_UUID_HHS_NOTI BT_UUID_DECLARE_128(BT_UUID_HHS_NOTI_VAL)
#define BT_UUID_HHS_WRITE BT_UUID_DECLARE_128(BT_UUID_HHS_WRITE_VAL)

/** Product : 10sec **/
#define TIMEOUT_SEC 10

/* Define a list of Bluetooth events with their corresponding values. */
#define BT_EVENT_LIST(X)                                               \
	/* event wait timeout(TIMEOUT_SEC) */                          \
	X(TIMEOUT, = 0x01)                                             \
	/* event ble notify enabled */                                 \
	X(BLE_NOTIFY_EN, = 0x02)                                       \
	/* event gas sensor value exceeding the threshold(O2_THRES) */ \
	X(GAS_VAL_CHANGE, = 0x04)                                      \
	/* event iaq value exceeding the threshold(IAQ_VAL_THRESH) */  \
	X(IAQ_VAL_THRESH, = 0x08)                                      \
	/* event voc value exceeding the threshold(VOC_VAL_THRESH) */  \
	X(VOC_VAL_THRESH, = 0x10)                                      \
	/* event co2 value exceeding the threshold(CO2_VAL_THRESH) */  \
	X(CO2_VAL_THRESH, = 0x20)
DECLARE_ENUM(bt_tx_event, BT_EVENT_LIST)

extern struct k_event bt_event;

int bt_setup(void);

#endif // __APP_BT_H__
