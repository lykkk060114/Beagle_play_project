/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-12
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-12
 * @FilePath: /beagle_play/extern/rtos_apps/include/sensor_node/get_sensor.h
 * @Description: Freedom传感器读取和UDP发送辅助函数
 *
 * Copyright (c) 2026  All Rights Reserved.
 */

#ifndef SENSOR_NODE_GET_SENSOR_H
#define SENSOR_NODE_GET_SENSOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/net/socket.h>

#include <sensor_node/types.h>

void update_latest_rssi(int8_t rssi);
int get_latest_rssi(void);

int setup_ipv6_addr(void);
int create_udp_socket(struct sockaddr_in6 *dest_addr);
int create_control_socket(void);
bool receive_light_command(int sock, bool *light_on);

int read_hdc2010(const struct device *dev, struct sensor_data *data);
int read_opt3001(const struct device *dev, struct sensor_data *data);
void print_sensor_values(const struct sensor_data *data);

int build_payload(char *buf,
                  size_t buf_size,
                  const struct sensor_data *data,
                  int rssi,
                  int seq);

#endif // SENSOR_NODE_GET_SENSOR_H
