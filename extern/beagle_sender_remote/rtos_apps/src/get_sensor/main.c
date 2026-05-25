/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 17:10:35
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 22:43:09
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

static int abs_val2(int val2)
{
    if (val2 < 0) {
        return -val2;
    }

    return val2;
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

    printk("Freedom HDC2010 sensor example start\n");

    hdc2010_dev = device_get_binding(HDC2010_DEV_NAME);
    if (hdc2010_dev == NULL) {
        printk("Could not find device: %s\n", HDC2010_DEV_NAME);
        return 0;
    }

    printk("Device found: %s\n", HDC2010_DEV_NAME);

    while (1) {
        struct sensor_value temperature;
        struct sensor_value humidity;
        int ret;

        ret = sensor_sample_fetch(hdc2010_dev);
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

        print_temperature(&temperature);
        print_humidity(&humidity);
        printk("--------------------\n");

        k_sleep(K_SECONDS(1));
    }

    return 0;
}