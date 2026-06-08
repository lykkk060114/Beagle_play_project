/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 23:00:42
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 23:12:36
 * @FilePath: /beagle_sender_remote/rtos_apps/src/sensor_node/utils.c
 * @Description: 
 * 
 * Copyright (c) 2026  All Rights Reserved. 
 */
#include "utils.h"


bool fetch_sensor_data(const struct device *hdc2010_dev, const struct device *light_dev, my_SensorData *data) {
    const struct sensor_value *temperature;
    const struct sensor_value *humidity;
    const struct sensor_value *light;
    int ret;
    
    ret = sensor_sample_fetch(hdc2010_dev);
    ret *= sensor_sample_fetch(light_dev);
    if (ret < 0) {
        printk("sensor_sample_fetch failed: %d\n", ret);
        return false;  
    }

    int ret1, ret2, ret3;
    ret1 = sensor_channel_get(hdc2010_dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    ret2 = sensor_channel_get(hdc2010_dev, SENSOR_CHAN_HUMIDITY, &humidity);
    ret3 = sensor_channel_get(light_dev, SENSOR_CHAN_LIGHT, &light);

    if (ret1 < 0 || ret2 < 0 || ret3 < 0) {
        printk("sensor_channel_get failed: %d, %d, %d\n", ret1, ret2, ret3);
        return false;  
    }

    print_temperature(temperature);
    print_humidity(humidity);
    print_light(light);
    printk("--------------------\n");
    data->temperature = temperature->val1 + temperature->val2;
    data->humidity = humidity->val1 + humidity->val2;
    data->light = light->val1 + light->val2;

    return true;
}
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