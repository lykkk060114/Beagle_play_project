# Freedom RTOS Apps

这个目录是烧录到 Freedom 板子的 Zephyr RTOS 程序。

默认构建 F1：

```bash
/home/debian/extern/rtos_apps/scripts/flash_f1.sh
```

构建 F2：

```bash
/home/debian/extern/rtos_apps/scripts/flash_f2.sh
```

只构建不烧录：

```bash
/home/debian/extern/rtos_apps/scripts/build_f1.sh
/home/debian/extern/rtos_apps/scripts/build_f2.sh
```

F1 和 F2 使用同一份传感器代码，只通过 CMake 参数区分：

- `FREEDOM_NODE_ID`：JSON 里的 `node` 字段
- `FREEDOM_NODE_IPV6`：Freedom 板子的 IPv6 地址

BeaglePlay 网关按 JSON `node` 字段区分 F1/F2，不按源 IPv6 区分。
