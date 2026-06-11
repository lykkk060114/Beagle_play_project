# Beagle Gateway

这个目录只放 BeaglePlay Linux 侧程序。

目录分工：

- `src/freedom_recv/`：接收 Freedom 板子的 IPv6 UDP JSON，监听 `9999`
- `src/host_send/`：把 BeaglePlay 当前收到的传感器数据转发给主机，默认发到 `192.168.7.1:9000`
- `src/host_recv/`：接收主机给 BeaglePlay 的控制命令，默认监听 `9001`
- `src/control/`：根据节点温度和主机命令决定风扇/语音动作
- `src/actuators/`：风扇 PWM 和语音播放的底层控制
- `src/common/`：JSON、节点状态等公共工具

Freedom 烧录工程仍然在：

- `extern/rtos_apps/`

手动构建：

```bash
cd /home/debian/extern/beagle_gateway
cmake -S . -B build
cmake --build build
```

统一启动：

```bash
/home/debian/extern/scripts/beagle_bridge.sh start
```

这个脚本会先停止旧的分体通信程序，再构建并启动新的融合版 `beagle_gateway`。
