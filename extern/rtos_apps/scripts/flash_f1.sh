#!/usr/bin/env bash
set -e

ZEPHYR_DIR=${ZEPHYR_DIR:-/home/debian/zephyr-beagle-cc1352-sdk/zephyr}
BUILD_DIR=${BUILD_DIR:-build/rtos_f1}

"$(dirname "$0")/build_f1.sh"

cd "$ZEPHYR_DIR"
west flash -d "$BUILD_DIR"
