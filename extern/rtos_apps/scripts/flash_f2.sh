#!/usr/bin/env bash
set -e

ZEPHYR_DIR=${ZEPHYR_DIR:-/home/debian/zephyr-beagle-cc1352-sdk/zephyr}
BUILD_DIR=${BUILD_DIR:-build/rtos_f2}

export ZEPHYR_BASE="$ZEPHYR_DIR"
cd "$ZEPHYR_DIR"

RUNNERS_YAML="$BUILD_DIR/zephyr/runners.yaml"
if [ -f "$RUNNERS_YAML" ]; then
  sed -i "s#- /boards/arm/beagle_bcf/cc2538-bsl.py#- $ZEPHYR_DIR/boards/arm/beagle_bcf/cc2538-bsl.py#" "$RUNNERS_YAML"
fi

west flash -d "$BUILD_DIR"
