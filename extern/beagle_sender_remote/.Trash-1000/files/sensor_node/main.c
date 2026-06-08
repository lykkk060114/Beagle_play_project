/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 23:00:32
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 23:00:41
 * @FilePath: /beagle_sender_remote/rtos_apps/src/sensor_node/main.c
 * @Description: 
 * 
 * Copyright (c) 2026  All Rights Reserved. 
 */


#include "utils.h"
#define HDC2010_DEV_NAME "HDC2010-HUMIDITY"
#define LIGHT_DEV_NAME "OPT3001-LIGHT"

int main(void) {
    const struct device *hdc2010_dev;
    const struct device *light_dev;

    printk("Freedom sensor example start\n");

    hdc2010_dev = device_get_binding(HDC2010_DEV_NAME);
    light_dev = device_get_binding(LIGHT_DEV_NAME);
    if (hdc2010_dev == NULL || light_dev == NULL) {
        printk("Failed to get device binding\n");
        return -1;
    }

    while (1) {
        my_SensorData data;
        if (fetch_sensor_data(hdc2010_dev, light_dev, &data)) {
            // 这里可以处理获取到的数据，例如发送到远程服务器
            k_sleep(K_SECONDS(1));
        } else {
            printk("Failed to fetch sensor data\n");
            k_sleep(K_SECONDS(1));
        }
    }

    return 0;
}