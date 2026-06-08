/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 17:10:35
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-08 21:24:29
 * @FilePath: /beagle_play/extern/rtos_apps/src/get_sensor/main.c
 * @Description: 读取传感器的值并且通过udp发送出来
 * 
 * Copyright (c) 2026  All Rights Reserved. 
 */

// C standard library
#include <string.h>
#include <stdio.h>
#include <errno.h>

// 头文件
#include <sensor_node/utils.h>


static int abs_val2(int val2) {
    if (val2 < 0) {
        return -val2;
    }

    return val2;
}

static int setup_ipv6_addr(void) {
	struct net_if *iface = net_if_get_default();
	struct in6_addr my_addr = {0};
	
	if(iface == NULL) {
		printk("net_if get default failed\n");
		return -ENODEV;
	}

	// 将字符串形式的IPv6地址转换为二进制形式
	if (net_addr_pton(AF_INET6, FREEDOM_IPV6_ADDR, &my_addr) < 0) {
        printk("invalid IPv6 address: %s\n", FREEDOM_IPV6_ADDR);
        return -EINVAL;
    }

	net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);

	printk("Freedom IPv6 address: %s\n", FREEDOM_IPV6_ADDR);
    return 0;
}

static int create_udp_socket(struct sockaddr_in6 *dest_addr) {
	int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printk("socket creation failed: %d\n", errno);
		return -errno;
	}

	memset(dest_addr, 0, sizeof(*dest_addr));
	dest_addr->sin6_family = AF_INET6;
	dest_addr->sin6_port = htons(BEAGLE_PORT);

	if (inet_pton(AF_INET6, BEAGLE_IPV6_ADDR, &dest_addr->sin6_addr) != 1) {
		printk("inet_pton failed: %s\n", BEAGLE_IPV6_ADDR);
		close(sock);
		return -EINVAL;
	}

	printk("UDP target: [%s]:%d\n", BEAGLE_IPV6_ADDR, BEAGLE_PORT);
	return sock;
}
/**
 * @description: get temperature and humidity data from hdc2010 sensor
 * @param {device} *dev
 * @param {agri_sensor_data} *data
 * @return {*}
 */
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

/**
 * @description: get light_data 
 * @param {device} *dev
 * @param {sensor_data} *data
 * @return {*}
 */
static int read_opt3001(const struct device *dev, struct sensor_data *data) {
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
static void print_light(const struct sensor_value *light) {
	printk("light       = %d.%06d lux\n",
		   light->val1,
		   abs_val2(light->val2));
}

// static void print_temperature(const struct sensor_value *temperature)
// {
// 	// val1是整数部分，val2是小数部分，单位是百万分之一
//     printk("temperature = %d.%06d C\n",
//            temperature->val1,
//            abs_val2(temperature->val2));
// }

// static void print_humidity(const struct sensor_value *humidity)
// {
//     printk("humidity    = %d.%06d %%\n",
//            humidity->val1,
//            abs_val2(humidity->val2));
// }

static int build_payload(char *buf,
						size_t buf_size,
						const struct sensor_data *data,
						int seq) {
	    return snprintf(
        buf,
        buf_size,
        "{\"node\":\"%s\","
        "\"temperature\":%d.%06d,"
        "\"humidity\":%d.%06d,"
        "\"light\":%d.%06d,"
        "\"rssi\":-60,"
        "\"seq\":%d}",
        NODE_ID,
        data->temperature.val1,
        abs_val2(data->temperature.val2),
        data->humidity.val1,
        abs_val2(data->humidity.val2),
        data->light.val1,
        abs_val2(data->light.val2),
        seq
    );
}
int main(void)
{
    const struct device *hdc2010_dev;
	const struct device *opt3001_dev;
    printk("Freedom sensor example start\n");

	struct sockaddr_in6 dest_addr;
	int sock;
	int seq = 0;


    hdc2010_dev = device_get_binding(HDC2010_DEV_NAME);
	opt3001_dev = device_get_binding(LIGHT_DEV_NAME);
    if (hdc2010_dev == NULL || opt3001_dev == NULL) {
        printk("Could not find device: %s or %s\n", HDC2010_DEV_NAME, LIGHT_DEV_NAME);
        return 0;
    }

	if (setup_ipv6_addr() < 0) {
		printk("Failed to setup IPv6 address\n");
		return 0;
	}

	sock = create_udp_socket(&dest_addr);
	if (sock < 0) {
		printk("Failed to create UDP socket\n");
		return 0;
	}

    while (1) {
		struct sensor_data data;
		char payload[192];
        int ret;

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

		ret = build_payload(payload, sizeof(payload), &data, seq);
        if (ret <= 0 || ret >= sizeof(payload)) {
            printk("build_payload failed or truncated: %d\n", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

        ret = sendto(sock,
                     payload,
                     strlen(payload),
                     0,
                     (struct sockaddr *)&dest_addr,
                     sizeof(dest_addr));

        if (ret < 0) {
            printk("sendto failed: errno=%d\n", errno);
        } else {
            printk("sent: %s\n", payload);
            seq++;
        }

        k_sleep(K_SECONDS(1));
    }

    return 0;
}
