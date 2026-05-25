/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 23:01:07
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 23:01:07
 * @FilePath: /beagle_sender_remote/rtos_apps/include/sensor_node/types.h
 * @Description: 这里完成个人结构体的定义
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */


#ifndef SENSOR_NODE_TYPES_H
#define SENSOR_NODE_TYPES_H

typedef struct {
    float temperature = 0.0f;
    float humidity = 0.0f;
    float light = 0.0f;
} my_SensorData;
#endif // SENSOR_NODE_TYPES_H