/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 17:10:35
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-12
 * @FilePath: /beagle_play/extern/rtos_apps/src/get_sensor/get_sensor.c
 * @Description: Read Freedom sensors and build UDP JSON payload.
 *
 * Copyright (c) 2026  All Rights Reserved.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>

#include <sensor_node/get_sensor.h>
#include <sensor_node/utils.h>

#define UDP_TARGET_IPV6_ADDR "ff02::1"

static int8_t latest_rssi = -128;
static K_MUTEX_DEFINE(rssi_mutex);

void update_latest_rssi(int8_t rssi)
{
    k_mutex_lock(&rssi_mutex, K_FOREVER);
    latest_rssi = rssi;
    k_mutex_unlock(&rssi_mutex);
}

int get_latest_rssi(void)
{
    int rssi;

    k_mutex_lock(&rssi_mutex, K_FOREVER);
    rssi = latest_rssi;
    k_mutex_unlock(&rssi_mutex);

    return rssi;
}

static int abs_val2(int val2)
{
    if (val2 < 0) {
        return -val2;
    }

    return val2;
}

int setup_ipv6_addr(void)
{
    struct net_if *iface = net_if_get_default();
    struct in6_addr my_addr = {0};
    struct in6_addr mcast_addr = {0};

    if (iface == NULL) {
        printk("net_if get default failed\n");
        return -ENODEV;
    }

    if (net_addr_pton(AF_INET6, FREEDOM_IPV6_ADDR, &my_addr) < 0) {
        printk("invalid IPv6 address: %s\n", FREEDOM_IPV6_ADDR);
        return -EINVAL;
    }

    net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
    printk("Freedom IPv6 address: %s\n", FREEDOM_IPV6_ADDR);

    if (net_addr_pton(AF_INET6, UDP_TARGET_IPV6_ADDR, &mcast_addr) < 0) {
        printk("invalid multicast IPv6 address: %s\n", UDP_TARGET_IPV6_ADDR);
        return -EINVAL;
    }

    net_if_ipv6_maddr_add(iface, &mcast_addr);
    printk("Freedom multicast address joined: %s\n", UDP_TARGET_IPV6_ADDR);

    return 0;
}

int create_udp_socket(struct sockaddr_in6 *dest_addr)
{
    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        printk("socket creation failed: errno=%d\n", errno);
        return -errno;
    }

    memset(dest_addr, 0, sizeof(*dest_addr));
    dest_addr->sin6_family = AF_INET6;
    dest_addr->sin6_port = htons(BEAGLE_PORT);

    if (inet_pton(AF_INET6, UDP_TARGET_IPV6_ADDR, &dest_addr->sin6_addr) != 1) {
        printk("inet_pton failed: %s\n", UDP_TARGET_IPV6_ADDR);
        close(sock);
        return -EINVAL;
    }

    printk("UDP target: [%s]:%d\n", UDP_TARGET_IPV6_ADDR, BEAGLE_PORT);
    return sock;
}

int create_control_socket(void)
{
    struct sockaddr_in6 addr;
    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        printk("control socket creation failed: errno=%d\n", errno);
        return -errno;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(FREEDOM_CONTROL_PORT);

    if (net_addr_pton(AF_INET6, "::", &addr.sin6_addr) < 0) {
        close(sock);
        return -EINVAL;
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printk("control bind failed: errno=%d\n", errno);
        close(sock);
        return -errno;
    }

    printk("Freedom control listening on UDP %d\n", FREEDOM_CONTROL_PORT);
    return sock;
}

bool receive_light_command(int sock, bool *light_on)
{
    char buf[96];
    int ret;

    if (sock < 0 || light_on == NULL) {
        return false;
    }

    ret = recvfrom(sock, buf, sizeof(buf) - 1, MSG_DONTWAIT, NULL, NULL);
    if (ret < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printk("control recv failed: errno=%d\n", errno);
        }
        return false;
    }

    buf[ret] = '\0';
    if (strstr(buf, "\"node\":\"") != NULL &&
        strstr(buf, "\"node\":\"" NODE_ID "\"") == NULL) {
        return false;
    }

    if (strstr(buf, "\"light_on\":true") != NULL || strstr(buf, "\"light\":true") != NULL) {
        *light_on = true;
        printk("RX control light_on=true\n");
        return true;
    }

    if (strstr(buf, "\"light_on\":false") != NULL || strstr(buf, "\"light\":false") != NULL) {
        *light_on = false;
        printk("RX control light_on=false\n");
        return true;
    }

    printk("RX unknown control: %s\n", buf);
    return false;
}

int read_hdc2010(const struct device *dev, struct sensor_data *data)
{
    int ret;

    ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &data->temperature);
    if (ret < 0) {
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &data->humidity);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int read_opt3001(const struct device *dev, struct sensor_data *data)
{
    int ret;

    ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &data->light);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

void print_sensor_values(const struct sensor_data *data)
{
    printk("temperature = %d.%06d C\n",
           data->temperature.val1,
           abs_val2(data->temperature.val2));
    printk("humidity    = %d.%06d %%\n",
           data->humidity.val1,
           abs_val2(data->humidity.val2));
    printk("light       = %d.%06d lux\n",
           data->light.val1,
           abs_val2(data->light.val2));
}

int build_payload(char *buf,
                  size_t buf_size,
                  const struct sensor_data *data,
                  int rssi,
                  int seq)
{
    return snprintf(
        buf,
        buf_size,
        "{\"node\":\"%s\","
        "\"temperature\":%d.%06d,"
        "\"humidity\":%d.%06d,"
        "\"light\":%d.%06d,"
        "\"rssi\":%d,"
        "\"seq\":%d}",
        NODE_ID,
        data->temperature.val1,
        abs_val2(data->temperature.val2),
        data->humidity.val1,
        abs_val2(data->humidity.val2),
        data->light.val1,
        abs_val2(data->light.val2),
        rssi,
        seq);
}
