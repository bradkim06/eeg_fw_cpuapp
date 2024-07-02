/**
 * @file src/bluetooth.c
 *
 * @brief Bluetooth functionality for a gas sensor device.
 *
 * This file contains the implementation of Bluetooth functionality for a gas
 * sensor device. It includes Bluetooth stack initialization, advertising,
 * handling connection events, updating connection parameters, and sending gas
 * sensor data via Bluetooth notifications.
 *
 * @author bradkim06@gmail.com
 */
#include "bluetooth.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

/* Registers the HHS_BT module with the specified log level. */
LOG_MODULE_REGISTER(HHS_BT, CONFIG_APP_LOG_LEVEL);

/* ble advertising name */
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/**
 * Bluetooth advertisement data structure.
 *
 * This structure defines the Bluetooth advertisement data format,
 * including flags, device name, and other information.
 */
static const struct bt_data ad[] = {
	/* Set the flags for the advertisement data */
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* Set the device name for the advertisement data */
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/*
 * This is a static constant structure that contains the Bluetooth data.
 * It is used to define the Bluetooth data bytes and the UUID value.
 */
static const struct bt_data sd[] = {
	// Defines the Bluetooth data bytes and the UUID value.
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_HHS_VAL),
};

/* Representing a connection to a remote device (kernel API). */
struct bt_conn *my_conn = NULL;

/**
 * Update the Bluetooth connection's PHY (Physical Layer).
 *
 * This function updates the PHY parameters of a Bluetooth connection. It
 * specifies the preferred PHY options, including reception and transmission PHY
 * types. If the update fails, it logs an error.
 *
 * @param conn The Bluetooth connection to update.
 */
static void update_phy(struct bt_conn *conn)
{
	int err;

	/* Set the preferred physical layer options */
	const struct bt_conn_le_phy_param preferred_phy = {
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};

	/* Update the physical layer of the connection */
	err = bt_conn_le_phy_update(conn, &preferred_phy);

	/* Check for errors and log if necessary */
	if (err) {
		LOG_ERR("bt_conn_le_phy_update() returned %d", err);
		return;
	}
}

/*
 * Function: update_data_length
 * Description: This function updates the data length of the given connection.
 * Parameters:
 * 		conn - pointer to the connection to be updated
 * Return: None
 */
static void update_data_length(struct bt_conn *conn)
{
	int err;
	/* Define the data length parameters */
	struct bt_conn_le_data_len_param my_data_len = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};

	/* Update the data length of the connection */
	err = bt_conn_le_data_len_update(my_conn, &my_data_len);

	/* Check for errors */
	if (err) {
		LOG_ERR("data_len_update failed (err %d)", err);
	}
}

/* Implement callback function for MTU exchange */
static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
	if (!att_err) {
		uint16_t payload_mtu = bt_gatt_get_mtu(conn) -
				       3; // 3 bytes used for Attribute headers.
		LOG_INF("New MTU: %d bytes", payload_mtu);
	}
}

/**
 * @brief Update the Maximum Transmission Unit (MTU) for the Bluetooth
 * connection.
 *
 * This function initiates an MTU exchange with a Bluetooth connection. It sets
 * up the callback function for handling MTU negotiation. If the exchange fails,
 * it logs an error message.
 *
 * @param conn The Bluetooth connection to update the MTU for.
 */
static void update_mtu(struct bt_conn *conn)
{
	/* Create variable that holds callback for MTU negotiation */
	static struct bt_gatt_exchange_params exchange_params;

	/* Set the callback function for handling MTU negotiation */
	exchange_params.func = exchange_func;

	/* Initiate MTU exchange with the given Bluetooth connection */
	int err = bt_gatt_exchange_mtu(conn, &exchange_params);

	/* Check if the exchange was successful */
	if (err) {
		/* Log an error message if the exchange failed */
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}

/**
 * Callback function for handling Bluetooth connection events when a device is
 * successfully connected.
 *
 * This function is called when a Bluetooth connection is successfully
 * established. It logs the successful connection, retrieves and logs the
 * connection parameters such as connection interval, latency, and supervision
 * timeout, updates the PHY mode, and negotiates the data length and MTU.
 *
 * @param conn The Bluetooth connection.
 * @param err The error code (0 for successful connection).
 */
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	// Check if connection was successful
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	// Log successful connection
	LOG_INF("Connected");

	// Store connection in global variable
	my_conn = bt_conn_ref(conn);

	// Declare a structure to store the connection parameters
	struct bt_conn_info info;

	// Retrieve connection parameters
	err = bt_conn_get_info(conn, &info);

	// Check if retrieval was successful
	if (err) {
		LOG_ERR("bt_conn_get_info() returned %d", err);
		return;
	}

	// Calculate connection interval in milliseconds
	double connection_interval = info.le.interval * 1.25;

	// Calculate supervision timeout in milliseconds
	uint16_t supervision_timeout = info.le.timeout * 10;

	// Log connection parameters
	LOG_INF("Connection parameters: interval %.2f ms, latency %d intervals, "
		"timeout %d ms",
		connection_interval, info.le.latency, supervision_timeout);

	// Update the PHY mode
	update_phy(my_conn);

	// Update the data length and MTU
	update_data_length(my_conn);
	update_mtu(my_conn);
}

/**
 * Callback function for handling Bluetooth disconnection events.
 *
 * This function is called when a Bluetooth connection is disconnected. It logs
 * the disconnection along with the reason for the disconnection and releases
 * the reference to the connection object.
 *
 * @param conn   The Bluetooth connection that was disconnected.
 * @param reason The reason for the disconnection.
 */
static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)", reason);
	bt_conn_unref(my_conn);
}

/**
 * Callback function for handling Bluetooth Low Energy (LE) parameter updates.
 *
 * This function is called when the connection parameters for a Bluetooth LE
 * connection are updated. It logs the updated parameters, including the
 * connection interval, latency, and supervision timeout.
 *
 * @param conn      The Bluetooth connection for which the parameters were
 * updated.
 * @param interval  The updated connection interval.
 * @param latency   The updated latency.
 * @param timeout   The updated supervision timeout.
 */
void on_le_param_updated(struct bt_conn *conn, uint16_t interval,
			 uint16_t latency, uint16_t timeout)
{
	double connection_interval = interval * 1.25; // in ms
	uint16_t supervision_timeout = timeout * 10; // in ms
	LOG_INF("Connection parameters updated: interval %.2f ms, latency %d "
		"intervals, timeout %d "
		"ms",
		connection_interval, latency, supervision_timeout);
}

/* Write a callback function to inform about updates in the PHY */
void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	// PHY Updated
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
		LOG_INF("PHY updated. New PHY: 1M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
		LOG_INF("PHY updated. New PHY: 2M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
		LOG_INF("PHY updated. New PHY: Long Range");
	}
}

/* Write a callback function to inform about updates in data length */
void on_le_data_len_updated(struct bt_conn *conn,
			    struct bt_conn_le_data_len_info *info)
{
	uint16_t tx_len = info->tx_max_len;
	uint16_t tx_time = info->tx_max_time;
	uint16_t rx_len = info->rx_max_len;
	uint16_t rx_time = info->rx_max_time;
	LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us",
		tx_len, rx_len, tx_time, rx_time);
}

struct bt_conn_cb connection_callbacks = {
	/* callback for ble successfully connected */
	.connected = on_connected,
	/* callback for ble successfully disconnected */
	.disconnected = on_disconnected,
	/* callback for connection parameter updates */
	.le_param_updated = on_le_param_updated,
	/* callback for PHY mode updates */
	.le_phy_updated = on_le_phy_updated,
	/* callback for data length updates */
	.le_data_len_updated = on_le_data_len_updated,
};

/**
 * Initialize and configure Bluetooth functionality.
 *
 * This function sets up the Bluetooth stack, configures advertising parameters,
 * and starts advertising. It also registers connection callbacks for handling
 * Bluetooth connection events.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int bt_setup(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}
	bt_conn_cb_register(&connection_callbacks);

	LOG_INF("Bluetooth initialized");

	struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
		(BT_LE_ADV_OPT_CONNECTABLE |
		 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use
                                         identity address */
		3200, /* Min Advertising Interval 500ms (800*0.625ms) */
		3201, /* Max Advertising Interval 2000ms (3200*0.625ms) */
		NULL); /* Set to NULL for undirected advertising */

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}

	LOG_INF("Advertising successfully started");

	return 0;
}

SYS_INIT(bt_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* Flag for transmitting only when notify is enabled. */
static bool bt_notify_enable = false;

/**
 * @brief Callback function for Gas Sensor CCC (Client Characteristic Configuration) changes.
 *
 * This function is invoked when the CCC descriptor of the Gas Sensor characteristic is modified
 * on the client side. It updates the notify_gas_enabled flag and posts a BLE_NOTIFY_EN event
 * if notifications are enabled.
 *
 * @param attr The BT GATT attribute that has been changed.
 * @param value The new value of the CCC descriptor.
 */
static void mylbsbc_ccc_gas_cfg_changed(const struct bt_gatt_attr *attr,
					uint16_t value)
{
	/* Update the notify_gas_enabled flag */
	bt_notify_enable = (value == BT_GATT_CCC_NOTIFY);

	/* Log the change in the CCC descriptor */
	LOG_INF("notify cfg changed %d", bt_notify_enable);
}

// Define a function to handle BLE (Bluetooth Low Energy) write operations.
// This function is called when a BLE client writes to a characteristic on the BLE server.
/**
 * @brief GATT write callback in a Bluetooth stack
 *
 * @param conn A pointer to the BLE connection structure.
 * @param attr A pointer to the GATT attribute that is being written to.
 * @param buf A pointer to the buffer containing the data to be written.
 * @param len The length of the data in the buffer.
 * @param offset The offset at which the data should be written (used for long writes).
 * @param flags Flags for the write operation (not used in this snippet).
 * @return the length of the data written if the write operation is successful.
 */
static ssize_t write_ble(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	// Log a debug message with the attribute handle and the connection pointer.
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	// Return the number of bytes written to indicate success.
	return len;
}

/* Service Declaration */
BT_GATT_SERVICE_DEFINE(
	bt_hhs_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_HHS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HHS_WRITE, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_ble, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HHS_NOTI, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(mylbsbc_ccc_gas_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static int bt_notify(char *data)
{
	char log_string[sizeof("notify data of length: 999") + 1];
	uint8_t data_length = strlen(data);

	return bt_gatt_notify(NULL, &bt_hhs_svc.attrs[4], (void *)data,
			      (size_t)data_length);
}

/**
 * @brief Bluetooth thread function.
 *
 * This function is the entry point for the Bluetooth thread. It initializes the Bluetooth stack,
 * starts advertising, and handles Bluetooth notifications. It also converts the firmware build time
 * to the time_t format for adding it to the current kernel time (k_uptime_get()). It periodically
 * sends notifications if a client is subscribed and gas sensor data is available. The function
 * constructs the notification data, including gas sensor values, battery percentage, and, if
 * enabled, environmental sensor data (temperature, pressure, humidity, IAQ, eCO2, and breath VOC).
 *
 * @note The function sends notifications only if a client is subscribed.
 */
static void bluetooth_thread(void)
{
}

#define STACKSIZE 2048
#define PRIORITY 0
K_THREAD_DEFINE(bt_thread_id, STACKSIZE, bluetooth_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
