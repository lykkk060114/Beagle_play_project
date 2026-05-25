/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-25 15:37:47
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 15:37:48
 * @FilePath: /beagle_play/extern/rtos_apps/src/test_rtos/main.c
 * @Description: 
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void) {
    int count = 0;
    while(1) {

        printk("rtos is running...\n");
        count++;
        k_sleep(K_MSEC(1000));
        if (count >= 100) {
            break;
        }
    }
    return 0;
}