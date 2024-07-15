#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BMI270, CONFIG_APP_LOG_LEVEL);

#define BMI270_NODE DT_NODELABEL(bmi270)

static const struct device *bmi270_dev = DEVICE_DT_GET(BMI270_NODE);
static struct k_sem data_ready_sem;

static void bmi270_trigger_handler(const struct device *bmi270_dev,
				   const struct sensor_trigger *trig)
{
	k_sem_give(&data_ready_sem);
}

void bmi270_thread(void)
{
	int ret;
	struct sensor_value accel[3], gyro[3];
	struct sensor_value full_scale, sampling_freq, oversampling;

	if (!device_is_ready(bmi270_dev)) {
		LOG_ERR("BMI270 device not ready\n");
		return;
	}

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 2; /* G */
	full_scale.val2 = 0;
	/* NOTE: See Detail frequency range (P.32) */
	sampling_freq.val1 = 100; /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1; /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(bmi270_dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_FULL_SCALE, &full_scale);
	sensor_attr_set(bmi270_dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_OVERSAMPLING, &oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(bmi270_dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

	/* Setting scale in degrees/s to match the sensor scale */
	full_scale.val1 = 500; /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100; /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1; /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(bmi270_dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_FULL_SCALE, &full_scale);
	sensor_attr_set(bmi270_dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_OVERSAMPLING, &oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change sampling frequency to
	 * 0.0Hz before changing other attributes
	 */
	sensor_attr_set(bmi270_dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

	k_sem_init(&data_ready_sem, 0, 1);

	// Configure interrupt
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	ret = sensor_trigger_set(bmi270_dev, &trig, bmi270_trigger_handler);
	if (ret != 0) {
		printk("Failed to set trigger: %d\n", ret);
		return;
	}

	while (1) {
		k_sem_take(&data_ready_sem, K_FOREVER);

		ret = sensor_sample_fetch(bmi270_dev);
		if (ret != 0) {
			LOG_ERR("Failed to fetch sample: %d\n", ret);
			continue;
		}

		sensor_channel_get(bmi270_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
		sensor_channel_get(bmi270_dev, SENSOR_CHAN_GYRO_XYZ, gyro);

		// Process and print data
		LOG_INF("AX: %d.%06d; AY: %d.%06d; AZ: %d.%06d; "
			"GX: %d.%06d; GY: %d.%06d; GZ: %d.%06d;",
			accel[0].val1, accel[0].val2, accel[1].val1,
			accel[1].val2, accel[2].val1, accel[2].val2,
			gyro[0].val1, gyro[0].val2, gyro[1].val1, gyro[1].val2,
			gyro[2].val1, gyro[2].val2);
	}
}

#define BMI270_STACKSIZE 2048
#define BMI270_PRIORITY 2
K_THREAD_DEFINE(bmi270_thread_id, BMI270_STACKSIZE, bmi270_thread, NULL, NULL,
		NULL, BMI270_PRIORITY, 0, 0);
