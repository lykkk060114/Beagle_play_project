/***
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-12 06:02:11
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-12 06:02:12
 * @FilePath: /beagle_play/extern/rtos_apps/include/oled_show/oled_show.h
 * @Description: oled显示的头文件
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#ifndef OLED_SHOW_H
#define OLED_SHOW_H

#include <stdbool.h>

#include <sensor_node/types.h>

bool oled_show_init(void);
void oled_set_light(bool enabled);
void oled_show_sensor(const struct sensor_data *data, int rssi, int seq);

#endif // OLED_SHOW_H
