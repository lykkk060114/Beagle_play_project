#!/usr/bin/env bash
set -e

ZEPHYR_DIR=${ZEPHYR_DIR:-/home/debian/zephyr-beagle-cc1352-sdk/zephyr}
BUILD_DIR=${BUILD_DIR:-build/rtos_f1}
cd "$ZEPHYR_DIR"

west build \
  -b beagleconnect_freedom \
  /home/debian/extern/rtos_apps \
  -d "$BUILD_DIR" \
  --pristine \
  -- \
  -DFREEDOM_NODE_ID=F1 \
  -DFREEDOM_NODE_IPV6=2001:db8::1
