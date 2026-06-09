#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define HDC2010_DEV_NAME "HDC2010-HUMIDITY"
#define OPT3001_DEV_NAME "OPT3001-LIGHT"
#define NODE_ID "F1"

struct agri_sensor_data {
	struct sensor_value temperature;
	struct sensor_value humidity;
	struct sensor_value light;
};

static int abs_val2(int val2)
{
	if (val2 < 0) {
		return -val2;
	}

	return val2;
}

static int read_hdc2010(const struct device *dev, struct agri_sensor_data *data)
{
	int ret;

	/* Fetch once, then read both cached HDC2010 sensor channels. */
	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
				 &data->temperature);
	if (ret < 0) {
		return ret;
	}

	return sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &data->humidity);
}

static int read_opt3001(const struct device *dev, struct agri_sensor_data *data)
{
	int ret;

	/* Fetch a fresh OPT3001 sample before reading the light channel. */
	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		return ret;
	}

	return sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &data->light);
}

static void print_agri_data(const struct agri_sensor_data *data)
{
	printk("agri_data node=%s temperature=%d.%06d humidity=%d.%06d light=%d.%06d\n",
	       NODE_ID,
	       data->temperature.val1, abs_val2(data->temperature.val2),
	       data->humidity.val1, abs_val2(data->humidity.val2),
	       data->light.val1, abs_val2(data->light.val2));
}

int main(void)
{
	const struct device *hdc2010_dev;
	const struct device *opt3001_dev;

	printk("Freedom agri sensor app start\n");

	hdc2010_dev = device_get_binding(HDC2010_DEV_NAME);
	if (hdc2010_dev == NULL) {
		printk("device_get_binding failed: %s\n", HDC2010_DEV_NAME);
		return 0;
	}

	opt3001_dev = device_get_binding(OPT3001_DEV_NAME);
	if (opt3001_dev == NULL) {
		printk("device_get_binding failed: %s\n", OPT3001_DEV_NAME);
		return 0;
	}

	printk("Device found: %s\n", HDC2010_DEV_NAME);
	printk("Device found: %s\n", OPT3001_DEV_NAME);

	while (1) {
		struct agri_sensor_data data;
		int ret;

		ret = read_hdc2010(hdc2010_dev, &data);
		if (ret < 0) {
			printk("read_hdc2010 failed: %d\n", ret);
			k_sleep(K_SECONDS(1));
			continue;
		}

		ret = read_opt3001(opt3001_dev, &data);
		if (ret < 0) {
			printk("read_opt3001 failed: %d\n", ret);
			k_sleep(K_SECONDS(1));
			continue;
		}

		print_agri_data(&data);
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
