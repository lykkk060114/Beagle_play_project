/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 17:10:35
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-09
 * @FilePath: /beagle_play/extern/rtos_apps/src/get_sensor/main.c
 * @Description: Read Freedom sensors and send JSON over IPv6 UDP.
 *
 * Copyright (c) 2026  All Rights Reserved.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sensor_node/utils.h>
#include <oled_show/oled_show.h>

#define UDP_TARGET_IPV6_ADDR "ff02::1"

static int8_t latest_rssi = -128;
static K_MUTEX_DEFINE(rssi_mutex);

void update_latest_rssi(int8_t rssi)
{
    k_mutex_lock(&rssi_mutex, K_FOREVER);
    latest_rssi = rssi;
    k_mutex_unlock(&rssi_mutex);
}

static int abs_val2(int val2)
{
    if (val2 < 0) {
        return -val2;
    }

    return val2;
}

static int setup_ipv6_addr(void)
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

static int create_udp_socket(struct sockaddr_in6 *dest_addr)
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

static int read_hdc2010(const struct device *dev, struct sensor_data *data)
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

static int read_opt3001(const struct device *dev, struct sensor_data *data)
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

static void print_sensor_values(const struct sensor_data *data)
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

static int build_payload(char *buf,
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

int main(void)
{
    const struct device *hdc2010_dev;
    const struct device *opt3001_dev;
    struct sockaddr_in6 dest_addr;
    int sock;
    int seq = 1;

    printk("Freedom JSON UDP sensor app start\n");

    hdc2010_dev = device_get_binding(HDC2010_DEV_NAME);
    opt3001_dev = device_get_binding(LIGHT_DEV_NAME);
    if (hdc2010_dev == NULL || opt3001_dev == NULL) {
        printk("Could not find device: %s or %s\n",
               HDC2010_DEV_NAME,
               LIGHT_DEV_NAME);
        return 0;
    }

    printk("Sensor devices ready: %s, %s\n", HDC2010_DEV_NAME, LIGHT_DEV_NAME);
    oled_show_init();

    if (setup_ipv6_addr() < 0) {
        printk("Failed to setup IPv6 address\n");
        return 0;
    }

    sock = create_udp_socket(&dest_addr);
    if (sock < 0) {
        printk("Failed to create UDP socket\n");
        return 0;
    }

    k_sleep(K_SECONDS(2));

    while (1) {
        struct sensor_data data;
        char payload[192];
        int rssi;
        int ret;

        memset(&data, 0, sizeof(data));

        ret = read_hdc2010(hdc2010_dev, &data);
        if (ret < 0) {
            printk("read_hdc2010 failed: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

        ret = read_opt3001(opt3001_dev, &data);
        if (ret < 0) {
            printk("read_opt3001 failed: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

        print_sensor_values(&data);

        k_mutex_lock(&rssi_mutex, K_FOREVER);
        rssi = latest_rssi;
        k_mutex_unlock(&rssi_mutex);

        ret = build_payload(payload, sizeof(payload), &data, rssi, seq);
        if (ret <= 0 || ret >= sizeof(payload)) {
            printk("build_payload failed or truncated: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

        printk("JSON payload: %s\n", payload);
        oled_show_sensor(&data, rssi, seq);

        ret = sendto(sock,
                     payload,
                     strlen(payload),
                     0,
                     (struct sockaddr *)&dest_addr,
                     sizeof(dest_addr));

        if (ret < 0) {
            printk("sendto failed: errno=%d\n", errno);
        } else {
            printk("sendto success: bytes=%d seq=%d\n", ret, seq);
            seq++;
        }

        k_sleep(K_SECONDS(1));
    }

    return 0;
}
