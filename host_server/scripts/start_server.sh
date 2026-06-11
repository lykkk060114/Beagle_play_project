#!/usr/bin/env bash
set -e
###
 # @Author: LYK && 2586356361@qq.com
 # @Date: 2026-06-11 20:13:22
 # @LastEditors: LYK && 2586356361@qq.com
 # @LastEditTime: 2026-06-11 20:20:41
 # @FilePath: /beagle_play/host_server/scripts/start_server.sh
 # @Description: 
 # 
 # Copyright (c) 2026  All Rights Reserved. 
### 

HOST_DIR="$(cd "$(dirname "$0")/.." && pwd)"
exec "$HOST_DIR/build/dashboard_server"
