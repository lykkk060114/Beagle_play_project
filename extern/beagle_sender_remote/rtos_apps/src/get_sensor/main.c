/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 17:10:35
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 22:58:16
 * @FilePath: /beagle_sender_remote/rtos_apps/src/get_sensor/main.c
 * @Description: 正式开始rtos例程的书写
 * 
 * Copyright (c) 2026  All Rights Reserved. 
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define HDC2010_DEV_NAME "HDC2010-HUMIDITY"
#define LIGHT_DEV_NAME "OPT3001-LIGHT"
static int abs_val2(int val2)
{
    if (val2 < 0) {
        return -val2;
    }

    return val2;
}

static void print_light(const struct sensor_value *light)
{
	printk("light       = %d.%06d lux\n",
		   light->val1,
		   abs_val2(light->val2));
}

static void print_temperature(const struct sensor_value *temperature)
{
	// val1是整数部分，val2是小数部分，单位是百万分之一
    printk("temperature = %d.%06d C\n",
           temperature->val1,
           abs_val2(temperature->val2));
}

static void print_humidity(const struct sensor_value *humidity)
{
    printk("humidity    = %d.%06d %%\n",
           humidity->val1,
           abs_val2(humidity->val2));
}

int main(void)
{
    const struct device *hdc2010_dev;

	const struct device *light_dev;
    printk("Freedom sensor example start\n");

    hdc2010_dev = device_get_binding(HDC2010_DEV_NAME);
	light_dev = device_get_binding(LIGHT_DEV_NAME);
    if (hdc2010_dev == NULL || light_dev == NULL) {
        printk("Could not find device: %s\n", HDC2010_DEV_NAME);
        return 0;
    }

    printk("Device found: %s\n", HDC2010_DEV_NAME);
    printk("Device found: %s\n", LIGHT_DEV_NAME);

    while (1) {
        struct sensor_value temperature;
        struct sensor_value humidity;
		struct sensor_value light;
        int ret;

		// 从传感器中拿数据
        ret = sensor_sample_fetch(hdc2010_dev);
		ret *= sensor_sample_fetch(light_dev);
        if (ret < 0) {
			printk("sensor_sample_fetch failed: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

		// 温度传感器通道
        ret = sensor_channel_get(hdc2010_dev,
                                 SENSOR_CHAN_AMBIENT_TEMP,
                                 &temperature);
        if (ret < 0) {
            printk("sensor_channel_get temperature failed: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

		// 湿度传感器通道
        ret = sensor_channel_get(hdc2010_dev,
                                 SENSOR_CHAN_HUMIDITY,
                                 &humidity);
        if (ret < 0) {
            printk("sensor_channel_get humidity failed: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

		ret = sensor_channel_get(light_dev,
								 SENSOR_CHAN_LIGHT,
								 &light);

		if (ret < 0) {
			printk("sensor_channel_get light failed: %d\n", ret);
			k_sleep(K_SECONDS(1));
			continue;
		}

        print_temperature(&temperature);
        print_humidity(&humidity);
        print_light(&light);
        printk("--------------------\n");

        k_sleep(K_SECONDS(1));
    }

    return 0;
}