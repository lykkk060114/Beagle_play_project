/*
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-12 06:02:32
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-12 06:02:33
 * @FilePath: /beagle_play/extern/rtos_apps/src/oled/oled_show.c
 * @Description: oled的显示程序
 *
 * Copyright (c) 2026  All Rights Reserved. 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <oled_show/oled_show.h>

#ifndef NODE_ID
#define NODE_ID "F1"
#endif

#define OLED_NODE DT_NODELABEL(ssd1306)
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGE_COUNT (OLED_HEIGHT / 8)
#define OLED_BUF_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)
#define OLED_STACK_SIZE 2048
#define OLED_PRIORITY 7

static const struct i2c_dt_spec oled_i2c = I2C_DT_SPEC_GET(OLED_NODE);
static uint8_t oled_buf[OLED_BUF_SIZE];
static K_THREAD_STACK_DEFINE(oled_stack, OLED_STACK_SIZE);
static struct k_thread oled_thread;
static K_MUTEX_DEFINE(oled_lock);
static bool oled_thread_started;
static bool oled_light_on;
static bool oled_has_latest_data;
static struct sensor_data oled_latest_data;
static int oled_latest_rssi;
static int oled_latest_seq;

static int abs_val2(int val2)
{
    if (val2 < 0) {
        return -val2;
    }

    return val2;
}

static const uint8_t *font_for(char ch)
{
    static const uint8_t space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t minus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t percent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};
    static const uint8_t digits[10][5] = {
        {0x3e, 0x51, 0x49, 0x45, 0x3e},
        {0x00, 0x42, 0x7f, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46},
        {0x21, 0x41, 0x45, 0x4b, 0x31},
        {0x18, 0x14, 0x12, 0x7f, 0x10},
        {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3c, 0x4a, 0x49, 0x49, 0x30},
        {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36},
        {0x06, 0x49, 0x49, 0x29, 0x1e},
    };
    static const uint8_t c[5] = {0x3e, 0x41, 0x41, 0x41, 0x22};
    static const uint8_t d[5] = {0x7f, 0x41, 0x41, 0x22, 0x1c};
    static const uint8_t e[5] = {0x7f, 0x49, 0x49, 0x49, 0x41};
    static const uint8_t f[5] = {0x7f, 0x09, 0x09, 0x09, 0x01};
    static const uint8_t h[5] = {0x7f, 0x08, 0x08, 0x08, 0x7f};
    static const uint8_t i[5] = {0x00, 0x41, 0x7f, 0x41, 0x00};
    static const uint8_t l[5] = {0x7f, 0x40, 0x40, 0x40, 0x40};
    static const uint8_t n[5] = {0x7f, 0x02, 0x04, 0x08, 0x7f};
    static const uint8_t o[5] = {0x3e, 0x41, 0x41, 0x41, 0x3e};
    static const uint8_t r[5] = {0x7f, 0x09, 0x19, 0x29, 0x46};
    static const uint8_t s[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
    static const uint8_t t[5] = {0x01, 0x01, 0x7f, 0x01, 0x01};

    if (ch >= '0' && ch <= '9') {
        return digits[ch - '0'];
    }

    switch (ch) {
    case '-':
        return minus;
    case '.':
        return dot;
    case '%':
        return percent;
    case 'C':
        return c;
    case 'D':
        return d;
    case 'E':
        return e;
    case 'F':
        return f;
    case 'H':
        return h;
    case 'I':
        return i;
    case 'L':
        return l;
    case 'N':
        return n;
    case 'O':
        return o;
    case 'R':
        return r;
    case 'S':
        return s;
    case 'T':
        return t;
    default:
        return space;
    }
}

static void draw_char(int x, int y, char ch)
{
    const uint8_t *font = font_for(ch);

    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            int px = x + col;
            int py = y + row;

            if (px < 0 || px >= OLED_WIDTH || py < 0 || py >= OLED_HEIGHT) {
                continue;
            }

            if ((font[col] & BIT(row)) != 0) {
                oled_buf[(py / 8) * OLED_WIDTH + px] |= BIT(py % 8);
            }
        }
    }
}

static void draw_pixel(int x, int y)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }

    oled_buf[(y / 8) * OLED_WIDTH + x] |= BIT(y % 8);
}

static void draw_frame(void)
{
    for (int x = 0; x < OLED_WIDTH; x++) {
        draw_pixel(x, 0);
        draw_pixel(x, OLED_HEIGHT - 1);
    }

    for (int y = 0; y < OLED_HEIGHT; y++) {
        draw_pixel(0, y);
        draw_pixel(OLED_WIDTH - 1, y);
    }
}

static void draw_text(int x, int y, const char *text)
{
    while (*text != '\0' && x < OLED_WIDTH) {
        draw_char(x, y, *text);
        x += 6;
        text++;
    }
}

static void draw_value(char *line,
                       size_t line_size,
                       const char *name,
                       const struct sensor_value *value,
                       const char *unit)
{
    snprintf(line,
             line_size,
             "%s %d.%02d%s",
             name,
             value->val1,
             abs_val2(value->val2) / 10000,
             unit);
}

static int oled_cmds(const uint8_t *cmds, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t buf[] = {0x00, cmds[i]};

        if (i2c_write_dt(&oled_i2c, buf, sizeof(buf)) < 0) {
            return -1;
        }
    }

    return 0;
}

static int oled_data(const uint8_t *data, size_t len)
{
    uint8_t buf[OLED_WIDTH + 1];

    if (len > OLED_WIDTH) {
        return -1;
    }

    buf[0] = 0x40;
    memcpy(&buf[1], data, len);

    return i2c_write_dt(&oled_i2c, buf, len + 1);
}

static bool oled_show_ram(void);
static bool oled_flush(void);
static void oled_draw_sensor_page(const struct sensor_data *data, int rssi, int seq);
static void oled_task(void *p1, void *p2, void *p3);

static bool oled_force_all_on(void)
{
    static const uint8_t all_on_cmd[] = {
        0xa5, /* entire display on */
        0xaf, /* display on */
    };

    if (oled_cmds(all_on_cmd, sizeof(all_on_cmd)) < 0) {
        printk("OLED all-on command failed\n");
        return false;
    }

    return true;
}

static bool oled_show_ram(void)
{
    static const uint8_t show_cmd[] = {
        0xa4, /* resume RAM content */
        0xa6, /* normal display */
        0xaf, /* display on */
    };

    if (oled_cmds(show_cmd, sizeof(show_cmd)) < 0) {
        printk("OLED show RAM command failed\n");
        return false;
    }

    return true;
}

static bool oled_write_init_cmds(void)
{
    static const uint8_t init_cmds[] = {
        0xae,       /* display off */
        0x20, 0x02, /* page addressing */
        0xb0,
        0xc8,
        0x00,
        0x10,
        0x40,
        0x81, 0xff, /* contrast */
        0xa1,
        0xa6,
        0xa8, 0x3f,
        0xa4,
        0xd3, 0x00,
        0xd5, 0x80,
        0xd9, 0xf1,
        0xda, 0x12,
        0xdb, 0x40,
        0x8d, 0x14, /* charge pump */
        0xaf,       /* display on */
    };

    if (oled_cmds(init_cmds, sizeof(init_cmds)) < 0) {
        printk("OLED init commands failed\n");
        return false;
    }

    return true;
}

static bool oled_flush(void)
{
    for (int page = 0; page < OLED_PAGE_COUNT; page++) {
        uint8_t set_page[] = {
            (uint8_t)(0xb0 + page),
            0x00,
            0x10,
        };

        if (oled_cmds(set_page, sizeof(set_page)) < 0) {
            printk("OLED set page failed: %d\n", page);
            return false;
        }

        if (oled_data(&oled_buf[page * OLED_WIDTH], OLED_WIDTH) < 0) {
            printk("OLED write page failed: %d\n", page);
            return false;
        }
    }

    return true;
}

static void oled_draw_sensor_page(const struct sensor_data *data, int rssi, int seq)
{
    char line[24];

    ARG_UNUSED(rssi);

    memset(oled_buf, 0, sizeof(oled_buf));
    draw_frame();

    draw_text(4, 2, "NODE " NODE_ID);

    draw_value(line, sizeof(line), "T", &data->temperature, "C");
    draw_text(4, 16, line);

    draw_value(line, sizeof(line), "H", &data->humidity, "%");
    draw_text(4, 30, line);

    draw_value(line, sizeof(line), "L", &data->light, "");
    draw_text(4, 44, line);

    snprintf(line, sizeof(line), "SEQ %d", seq);
    draw_text(70, 2, line);

    if (oled_flush()) {
        oled_show_ram();
    }
}

bool oled_show_init(void)
{
    if (oled_thread_started) {
        return true;
    }

    oled_thread_started = true;
    k_thread_create(&oled_thread,
                    oled_stack,
                    K_THREAD_STACK_SIZEOF(oled_stack),
                    (k_thread_entry_t)oled_task,
                    NULL,
                    NULL,
                    NULL,
                    OLED_PRIORITY,
                    0,
                    K_NO_WAIT);
    printk("OLED task started\n");

    return true;
}

void oled_set_light(bool enabled)
{
    k_mutex_lock(&oled_lock, K_FOREVER);
    oled_light_on = enabled;
    k_mutex_unlock(&oled_lock);
}

void oled_show_sensor(const struct sensor_data *data, int rssi, int seq)
{
    k_mutex_lock(&oled_lock, K_FOREVER);
    oled_latest_data = *data;
    oled_latest_rssi = rssi;
    oled_latest_seq = seq;
    oled_has_latest_data = true;
    k_mutex_unlock(&oled_lock);
}

static void oled_task(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (!i2c_is_ready_dt(&oled_i2c)) {
        printk("OLED I2C bus is not ready\n");
        return;
    }

    if (!oled_write_init_cmds()) {
        return;
    }

    oled_show_ram();
    printk("OLED ready\n");

    while (1) {
        bool light_on;
        bool has_data;
        struct sensor_data data;
        int rssi;
        int seq;

        k_mutex_lock(&oled_lock, K_FOREVER);
        light_on = oled_light_on;
        has_data = oled_has_latest_data;
        data = oled_latest_data;
        rssi = oled_latest_rssi;
        seq = oled_latest_seq;
        k_mutex_unlock(&oled_lock);

        if (light_on) {
            oled_force_all_on();
        } else if (has_data) {
            oled_draw_sensor_page(&data, rssi, seq);
        } else {
            oled_show_ram();
        }

        k_sleep(K_MSEC(1000));
    }
}
