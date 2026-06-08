/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-08 21:16:29
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-08 21:16:47
 * @FilePath: /beagle_play/extern/rtos_apps/include/sensor_node/utils.h
 * @Description: 
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */
#ifndef SENSOR_NODE_UTILS_H
#define SENSOR_NODE_UTILS_H

#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include "types.h"

#ifndef NODE_ID
#define NODE_ID "F1"
#endif

#ifndef HDC2010_DEV_NAME
#define HDC2010_DEV_NAME "HDC2010-HUMIDITY"
#endif

#ifndef LIGHT_DEV_NAME
#define LIGHT_DEV_NAME "OPT3001-LIGHT"
#endif

#ifndef FREEDOM_IPV6_ADDR
#define FREEDOM_IPV6_ADDR "2001:db8::1"
#endif

#ifndef BEAGLE_IPV6_ADDR
#define BEAGLE_IPV6_ADDR "2001:db8::2"
#endif

#ifndef BEAGLE_PORT
#define BEAGLE_PORT 9999
#endif

static int setup_ipv6_addr(void);
static int create_udp_socket(struct sockaddr_in6 *dest_addr);
static int read_hdc2010(const struct device *dev, struct sensor_data *data);
static int read_opt3001(const struct device *dev, struct sensor_data *data);
static int build_payload(char *buf, size_t buf_size, const struct sensor_data *data, int seq);
static int abs_val2(int val2);
static void print_light(const struct sensor_value *light);

#endif // SENSOR_NODE_UTILS_H
