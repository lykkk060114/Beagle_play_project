/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-12 07:45:58
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-12 07:45:59
 * @FilePath: /beagle_play/extern/rtos_apps/src/main/main.c
 * @Description: 放置总的任务函数
 *
 * Copyright (c) 2026  All Rights Reserved. 
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>

#include <oled_show/oled_show.h>
#include <sensor_node/get_sensor.h>
#include <sensor_node/utils.h>

static void handle_light_control(int control_sock, bool *light_on)
{
    while (receive_light_command(control_sock, light_on)) {
        oled_set_light(*light_on);
    }
}

static void sleep_with_light_control(int control_sock, bool *light_on)
{
    for (int i = 0; i < 10; i++) {
        handle_light_control(control_sock, light_on);
        k_sleep(K_MSEC(100));
    }
}

int main(void)
{
    const struct device *hdc2010_dev;
    const struct device *opt3001_dev;
    struct sockaddr_in6 dest_addr;
    int sock;
    int control_sock;
    int seq = 1;
    bool light_on = false;

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

    control_sock = create_control_socket();
    if (control_sock < 0) {
        printk("Failed to create control socket\n");
    }

    k_sleep(K_SECONDS(2));

    while (1) {
        struct sensor_data data;
        char payload[192];
        int rssi;
        int ret;

        memset(&data, 0, sizeof(data));
        handle_light_control(control_sock, &light_on);

        ret = read_hdc2010(hdc2010_dev, &data);
        if (ret < 0) {
            printk("read_hdc2010 failed: %d\n", ret);
            sleep_with_light_control(control_sock, &light_on);
            continue;
        }

        ret = read_opt3001(opt3001_dev, &data);
        if (ret < 0) {
            printk("read_opt3001 failed: %d\n", ret);
            sleep_with_light_control(control_sock, &light_on);
            continue;
        }

        print_sensor_values(&data);

        handle_light_control(control_sock, &light_on);

        rssi = get_latest_rssi();

        ret = build_payload(payload, sizeof(payload), &data, rssi, seq);
        if (ret <= 0 || ret >= sizeof(payload)) {
            printk("build_payload failed or truncated: %d\n", ret);
            sleep_with_light_control(control_sock, &light_on);
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

        sleep_with_light_control(control_sock, &light_on);
    }

    return 0;
}
