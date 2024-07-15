#include "ti_ads1299_driver_spi.h"
#include "eeg.h"
#include "filter.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#define ADS1299_NODE DT_NODELABEL(ads1299)

LOG_MODULE_REGISTER(EEG, CONFIG_APP_LOG_LEVEL);

// ADS1299 관련 상수
const float VREF = 4.5f; // 기준 전압 (V)
const int GAIN = 24; // PGA 게인
const int RESOLUTION = 24; // ADC 해상도 (비트)

const struct device *ads1299_spi_dev = DEVICE_DT_GET(DT_NODELABEL(ads1299));

static K_SEM_DEFINE(drdy_sem, 0, 1);
static struct gpio_callback drdy_cb_data;

#define DATA_SIZE 15
#define RING_BUF_SIZE 1024

static uint8_t ring_buffer_data[RING_BUF_SIZE];
static struct ring_buf ring_buf;

K_SEM_DEFINE(data_ready_sem, 0, 1);

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

// float32_t adc_to_voltage(int32_t adc_code)
// {
//
// 	// 2의 보수 형태의 ADC 코드를 부호 있는 정수로 변환
// 	if (adc_code > 0x7FFFFF) {
// 		adc_code -= 0x1000000;
// 	}
//
// 	// 전압 계산 (오버플로우 방지를 위해 순서 조정)
// 	float32_t voltage = ((float32_t)adc_code * VREF) / (float32_t)GAIN;
// 	voltage /= (float32_t)(1 << (RESOLUTION - 1));
//
// 	return voltage;
// }

// ADS1299에서 읽은 24비트 ADC 값을 실제 전압으로 변환하는 함수
float32_t adc_to_voltage(int32_t adc_value)
{
	// ADC 값이 24비트 2의 보수 형식이므로, 부호 확장
	if (adc_value & 0x800000) {
		adc_value |= 0xFF000000;
	}

	// ADC 값을 전압으로 변환
	float32_t voltage =
		(float32_t)adc_value * (2 * VREF) / (pow(2, 23) - 1) / GAIN;

	return voltage;
}

void process_and_print_data(const uint8_t *data, size_t size)
{
	float32_t voltage[2] = { 0.0 };
	for (int channel = 0; channel < 2; channel++) {
		int32_t value = (data[3 * channel + 3] << 16) |
				(data[3 * channel + 4] << 8) |
				data[3 * channel + 5];

		float32_t volt = adc_to_voltage(value);
		voltage[channel] = filteringEEGData(volt, channel);
	}

	printk("%f\n", voltage[0]);
}

static void data_processing_thread(void *arg1, void *arg2, void *arg3)
{
	uint8_t data[DATA_SIZE];

	while (1) {
		k_sem_take(&data_ready_sem, K_FOREVER);

		while (ring_buf_get(&ring_buf, data, DATA_SIZE) == DATA_SIZE) {
			process_and_print_data(data, DATA_SIZE);
		}
	}
}

static void eeg_thread(void)
{
	int err = device_is_ready(ads1299_spi_dev);
	if (!err) {
		LOG_ERR("Error: SPI device is not ready, err: %d", err);
		return;
	}

	ads1299_init();
	ring_buf_init(&ring_buf, sizeof(ring_buffer_data), ring_buffer_data);

#define START 0x08
#define RDATAC 0x10
	ti_ads1299_command(ads1299_spi_dev, START);
	ti_ads1299_command(ads1299_spi_dev, RDATAC);

	uint8_t data[DATA_SIZE];

	while (1) {
		k_sem_take(&drdy_sem, K_FOREVER);
		if (ti_ads1299_read_data(ads1299_spi_dev, data, sizeof(data)) ==
		    0) {
			uint32_t bytes_written =
				ring_buf_put(&ring_buf, data, DATA_SIZE);
			if (bytes_written != DATA_SIZE) {
				LOG_WRN("Ring buffer full, data lost");
			}
			k_sem_give(&data_ready_sem);
		} else {
			LOG_ERR("Error reading data from ADS1299");
		}
	}
}

#define EEG_STACKSIZE 2048
#define EEG_PRIORITY 0
K_THREAD_DEFINE(eeg_thread_id, EEG_STACKSIZE, eeg_thread, NULL, NULL, NULL,
		EEG_PRIORITY, 0, 0);

#define PROCESSING_STACKSIZE 2048
#define PROCESSING_PRIORITY 1
K_THREAD_DEFINE(processing_thread_id, PROCESSING_STACKSIZE,
		data_processing_thread, NULL, NULL, NULL, PROCESSING_PRIORITY,
		0, 0);
