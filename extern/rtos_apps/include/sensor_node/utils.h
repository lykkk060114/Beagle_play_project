/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 23:00:58
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 23:07:35
 * @FilePath: /beagle_sender_remote/rtos_apps/include/sensor_node/utils.h
 * @Description: 
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#ifndef SENSOR_NODE_UTILS_H
#define SENSOR_NODE_UTILS_H
// 个人的结构体定义
#include "types.h"

// 常用的rtos头文件
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

// 入口函数
extern bool fetch_sensor_data(const struct device *hdc2010_dev, const struct device *light_dev, my_SensorData *data);

// 核心获取函数
void fetch_mudi_data(const struct device *mudi_dev, float *temperature, float *humidity);
void fetch_light_data(const struct device *light_dev, float *light);


void print_temperature(const struct sensor_value *temperature);
void print_humidity(const struct sensor_value *humidity);
void print_light(const struct sensor_value *light);

int abs_val2(int val2);
