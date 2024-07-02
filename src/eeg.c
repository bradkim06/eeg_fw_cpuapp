#include "ti_ads1299_driver_spi.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#define ADS1299_NODE DT_NODELABEL(ads1299)

/* Registers the HHS_BT module with the specified log level. */
LOG_MODULE_REGISTER(EEG, CONFIG_APP_LOG_LEVEL);

const struct device *ads1299_spi_dev = DEVICE_DT_GET(DT_NODELABEL(ads1299));

static K_SEM_DEFINE(drdy_sem, 0, 1);
static struct gpio_callback drdy_cb_data;

// DRDY 인터럽트 핸들러
static void drdy_handler(const struct device *dev, struct gpio_callback *cb,
			 uint32_t pins)
{
	k_sem_give(&drdy_sem);
}

static int ads1299_init(void)
{
	const struct device *drdy_gpio_dev;
	int ret;

	// DRDY GPIO 디바이스 가져오기
	drdy_gpio_dev = DEVICE_DT_GET(DT_GPIO_CTLR(ADS1299_NODE, drdy_gpios));
	if (!device_is_ready(drdy_gpio_dev)) {
		LOG_ERR("DRDY GPIO device not ready");
		return -ENODEV;
	}

	// DRDY 핀을 입력으로 구성
	ret = gpio_pin_configure(
		drdy_gpio_dev, DT_GPIO_PIN(ADS1299_NODE, drdy_gpios),
		GPIO_INPUT | DT_GPIO_FLAGS(ADS1299_NODE, drdy_gpios));
	if (ret != 0) {
		LOG_ERR("Error configuring DRDY pin: %d", ret);
		return ret;
	}

	// DRDY 인터럽트 설정
	ret = gpio_pin_interrupt_configure(
		drdy_gpio_dev, DT_GPIO_PIN(ADS1299_NODE, drdy_gpios),
		GPIO_INT_EDGE_FALLING);
	if (ret != 0) {
		LOG_ERR("Error configuring DRDY interrupt: %d", ret);
		return ret;
	}

	// 콜백 초기화 및 추가
	gpio_init_callback(&drdy_cb_data, drdy_handler,
			   BIT(DT_GPIO_PIN(ADS1299_NODE, drdy_gpios)));
	ret = gpio_add_callback(drdy_gpio_dev, &drdy_cb_data);
	if (ret != 0) {
		LOG_ERR("Error adding DRDY callback: %d", ret);
		return ret;
	}

	LOG_INF("ADS1299 DRDY interrupt initialized");
	return 0;
}

void print_data_hex(const uint8_t *data, size_t size)
{
	static int i = 1;
	char log_buffer[100] = { 0 }; // 충분한 크기의 버퍼 확보
	char *ptr = log_buffer;
	int remaining = sizeof(log_buffer);

	for (size_t i = 0; i < size && remaining > 0; i++) {
		int written = snprintf(ptr, remaining, "%02X ", data[i]);
		if (written > 0) {
			ptr += written;
			remaining -= written;
		} else {
			LOG_WRN("print data buffer overflow");
			break; // 버퍼 공간 부족
		}
	}

	// 마지막 공백 제거
	if (ptr > log_buffer && *(ptr - 1) == ' ') {
		*(ptr - 1) = '\0';
	}

	LOG_INF("Data read %d: %s", i, log_buffer);
#define MAX_COUNT 1000000
	i = (i + 1) % MAX_COUNT;
}

static void eeg_thread(void)
{
	int err = device_is_ready(ads1299_spi_dev);
	if (!err) {
		LOG_ERR("Error: SPI device is not ready, err: %d", err);
		return;
	}

	ads1299_init();

	ti_ads1299_config(ads1299_spi_dev);

	uint8_t data[15]; // 3 bytes status + 12 bytes data (4 channels * 3 bytes per channel)
#define START 0x08
#define RDATAC 0x10
	ti_ads1299_command(ads1299_spi_dev, START);
	ti_ads1299_command(ads1299_spi_dev, RDATAC);
	while (1) {
		k_sem_take(&drdy_sem, K_FOREVER);
		if (ti_ads1299_read_data(ads1299_spi_dev, data, sizeof(data)) ==
		    0) {
			print_data_hex(data, sizeof(data));
		} else {
			LOG_ERR("Error reading data from ADS1299");
		}
	}
}

#define STACKSIZE 2048
#define PRIORITY 0
K_THREAD_DEFINE(eeg_thread_id, STACKSIZE, eeg_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
