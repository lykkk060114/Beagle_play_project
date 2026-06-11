#!/usr/bin/env bash
set -e

cd /home/debian/zephyr-beagle-cc1352-sdk/zephyr
cmake --build build/rtos_f1
