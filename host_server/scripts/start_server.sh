#!/usr/bin/env bash
set -e
###
 # @Author: LYK && 2586356361@qq.com
 # @Date: 2026-06-11 20:13:22
 # @LastEditors: LYK && 2586356361@qq.com
 # @LastEditTime: 2026-06-11 23:47:48
 # @FilePath: /beagle_play/host_server/scripts/start_server.sh
 # @Description: 
 # 
 # Copyright (c) 2026  All Rights Reserved. 
### 
WS_DIR="/home/lyk/git_workplace/beagle_play"
HOST_DIR="${WS_DIR}/host_server"
exec "$HOST_DIR/build/dashboard_server"
