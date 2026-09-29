# NexLink STM32 固件

NEX_LINK 协议在 MCU 侧的四份固件实现。上位机（libusb/WinUSB + C# 库与工具）在 `../../pc-sdk`。

四个工程共用同一套协议和一个 NEX_LINK USB 类，按用途分成四份，差别集中在**运行环境、
USB 形态**和**外设负载**上。

> `Middlewares/ST/STM32_USB_Device_Library/Class/NEX_LINK/` 在每个工程里各有一份**拷贝**
> （不是共享目录）。修改协议层、发送队列或 USB 类时要四个工程逐个同步。

## 目录

| 工程 | 用途 | 运行环境 | 芯片 | USB 形态 | VID / PID | 产品串 |
|---|---|---|---|---|---|---|
| `nexlink_template` | 参考实现（RTOS），协议验证基准 | FreeRTOS | STM32F407VE | 复合设备 | 0x1d51 / 0x6067 | NexLinkV2-STM32F4 |
| `nexlink_template_bare` | 最小裸机实现（移植起点） | 裸机 | STM32F407VE | 复合设备 | 0x1d51 / 0x6068 | NexLinkV2-STM32F4-Bare |
| `nexlink_adapter` | 总线转接器：I2C/SPI/UART/GPIO 透传 | FreeRTOS | STM32F407VE | **单接口** | 0x1d51 / 0x6061 | NexLinkV2-Adapter |
| `nexlink_husb` | 整机：LCD + 传感器 + 图形库 | 裸机 | STM32F405RG / STM32F407VE | 复合设备 | 0x1d51 / 0x6066 | NexLink-PLAYER |

四个工程 PID 各不相同，同一台 PC 上可区分（改 PID 也能强制 Windows 重建设备实例）。

**怎么选**：往新板子移植、或想看协议最小用法 → `nexlink_template_bare`；需要带 RTOS 的
参考 → `nexlink_template`；要做总线透传 → `nexlink_adapter`；整机产品 → `nexlink_husb`。

## 协议层速览

- **帧格式**（`nexlink_proto.h` 的 `nl_packet_t`，little-endian）：8 字节头
  `magic(0xA5) | type | cmd | seq | length` + payload。
  `type`：`CMD(0x01)` 上位机→设备、`RESP(0x02)` 设备应答、`EVENT(0x03)` 设备主动推。
- **长度上限**：payload ≤ `NL_MAX_PAYLOAD`(1024)；组包时还有帧长检查 —— RTOS 工程卡
  `BUF_SIZE`(1024)（内存池单块大小），裸机工程卡 1280（发送队列一个槽的大小）。
- **命令**：`NexLinkCmd` 枚举。通用段如 `CMD_PING(0x0001)`、`CMD_GET_VERSION(0x0002)`；
  `CMD_USER_BASE(0x7F00)` 之后是各工程的厂商自定义段（husb 的自定义命令就用这一段）。
- **应答**：`send_resp_ok(cmd, seq, payload, len)` / `send_resp_err(cmd, seq, err)`，
  上位机按 `seq` 匹配（`nexlink_cmd()` 内部就是注册 waiter + 超时等待）。
- **事件**：`send_event(cmd, payload, len)` —— 心跳、日志、帧上传、UART 数据等设备主动推送。
- **图像两个方向**：上位机→设备用 `CMD_FRAME_BEGIN/DATA/END`（husb 用来往 LCD 刷图）；
  设备→上位机用 `EVT_FRAME_UPLOAD_BEGIN/DATA/END`（`upload_frame_upload()`）。
- **版本号**：`NL_VERSION_MAJOR/MINOR/PATCH/BUILD`，由 `CMD_GET_VERSION` 返回，
  上位机工具里显示的 "Version：x.y.z" 就是它。上位机 DLL 自己还有个独立的 `VERSION`
  （`pc-sdk/nexlink_usb/nexlink_core.h`），两者无关。

### 上位机交互流程 → 固件侧的两条约束

上位机的"连接"是：`nexlink_scan()` 找到设备 → `nexlink_open()`（开 WinUSB 接口 + 起 RX 线程）
→ **立刻发一次 `CMD_GET_VERSION`**（1000ms 超时、不重试）。由此：

1. **设备主动推的事件必须等上位机连上之后再发**。
   实现：`send_event()` 上的 `nexlink_host_connected()` 闸门（收到第一条合法 CMD 才置位，
   USB 复位 / 去配置时清零）。上位机没在读时发出的包会挂在 EP1 IN 上没人取走，
   `TxReady` 一直不恢复，之后所有发送都会被丢掉（包括命令应答）。
   `send_resp_*` 不受闸门限制。
2. **应答不能等主循环、也不能等任务调度**。`nexlink_tx_send()` 入队后要立刻尝试发一次，
   否则上位机第一条命令就可能超时。`nexlink_husb` 尤其明显：它在 `MX_USB_DEVICE_Init()`
   之后还有一大段外设 / LVGL 初始化才进 `while(1)`，而那时上位机已经枚举完成并发命令了。

## USB 形态

### 复合设备：`nexlink_template` / `nexlink_template_bare` / `nexlink_husb`

CDC 虚拟串口 + NEX_LINK 厂商接口。`USBD_MAX_NUM_INTERFACES = 3`（`usbd_conf.h`），
启用 `USE_USBD_COMPOSITE` + `usbd_composite_builder.c`。

- 接口：CDC comm(0) / CDC data(1) / NEX_LINK(2)
- 端点：NEX_LINK `0x81` IN / `0x01` OUT；CDC `0x82` notif / `0x83` IN / `0x03` OUT
- **注册顺序必须是 CDC 先、NEX_LINK 后**（NEX_LINK 靠"最后一个 Init"让 `pClassData`
  指向自己的句柄），见各工程 `main.c` 的 `MX_USB_DEVICE_Init()`
- MS OS 兼容 ID 描述符里的接口号是 **2**

### 单接口：`nexlink_adapter`

只有 NEX_LINK 一个 USB 类，**没有 CDC 虚拟串口**：

```c
USBD_RegisterClass(&hUSB, &USBD_NEX_LINK);   // 不是 RegisterClassComposite
```

`USBD_MAX_NUM_INTERFACES = 1`，不带 `USE_USBD_COMPOSITE`、不带复合构建；
接口号 **0**，端点 `0x81` / `0x01`。
（`USB_DEVICE/App/usbd_cdc_if.c` 等文件仍在磁盘上，但没加入编译。）

### 共同点

- **USB HS**：`OTG_HS` + ULPI 外部 PHY，`DEVICE_HS`，`dev_endpoints = 6`，
  HS Tx FIFO 按 4 个 IN 端点分配（`USB_DEVICE/Target/usbd_conf.c`）
- **Windows 侧免驱**靠 MS OS 1.0 描述符：`0xEE` 字符串描述符 + vendor 请求 `0x0004`
  （compatible ID = WINUSB）与 `0x0005`（DeviceInterfaceGUIDs），实现在 `usbd_nex_link.c`
- **每个 Keil 工程两个 target**，输出同名 `.axf/.bin`、共用输出目录：
  - `xxx`：从 `0x08000000` 直接运行
  - `xxx_app`：带 `VECT_TAB_OFFSET = 0x10000U`，配合 bootloader 运行在 `0x08010000`
    （`nexlink_template_bare` 的 app target 名字是 `nexlink_template_app_bare`）

## 代码结构（`Class/NEX_LINK/`）

| 文件 | 职责 |
|---|---|
| `nexlink_proto.h` | 帧格式、命令/事件枚举、错误码、版本号、长度上限。四个工程必须保持一致 |
| `usbd_nex_link.c/.h` | USB 类：配置描述符、端点开关、MS OS 描述符、发送原语 `USBD_NEX_LINK_TxReady/Transmit`、`DataOut` 接收回调 |
| `nexlink_app.c/.h` | 协议层：`nexlink_rx_bytes()` 拆包、`handle_cmd()` 分发、心跳、日志、帧上传；含 `__weak external_handle_cmd()` 挂载点 |
| `nexlink_tx.c/.h` | 发送队列（`nexlink_tx_send()` 入队 / `nexlink_tx_poll()` 递给 USB 栈） |
| `nexlink_usb_if.c/.h` | 协议层与 USB 类之间的接口（`usb_rx_isr()` / `usb_tx()`） |
| `nexlink_lock.h` | RTOS 工程：中断/任务通用的临界区（按 `__get_IPSR()` 分派） |
| `nexlink_tasks.c`、`nexlink_ringbuf.c`、`nexlink_rampool.c` | RTOS 专属：任务创建、RX 环形缓冲、TX 缓冲池。仅 `nexlink_template` / `nexlink_adapter` |
| `nexlink_external.c` | `nexlink_adapter` 专属：I2C/SPI/UART/GPIO 透传命令实现 |

## 收发模型（两种）

| | 任务模型（`nexlink_template` / `nexlink_adapter`） | 中断模型（`nexlink_template_bare` / `nexlink_husb`） |
|---|---|---|
| RX | USB 中断只 `ringbuf_write_from_isr()` 并唤醒任务；拆包和 `handle_cmd()` 在 RX 任务里跑 | USB 中断里直接 `nexlink_rx_bytes()` → `handle_cmd()` |
| TX | `buf_alloc()` 取池缓冲、**直接在池缓冲里组包（零拷贝）** → 指针环 → `NexLinkTxTask` 循环 `nexlink_tx_poll()` | 栈上组包 → 字节环（4 × 1280，拷贝一次）→ 主循环 `nexlink_tx_poll()` |
| 帧缓冲回收 | `TxReady` 恢复后 `buf_free()` 归还内存池 | 静态数组，无需回收 |

由此决定 `handle_cmd()` 里能放多少活：

- 任务模型：可以做稍慢的事（I2C 事务、大块数据处理），不拖 USB 中断。
- 中断模型：`handle_cmd()` 必须快。重活要挪到主循环 —— husb 的 `CMD_FRAME_GET` 应答就是
  先记下 `cmd/seq`、由主循环里的 `upload_frame_upload()` 再回包的。

### 改发送路径时必须保持的性质

`nexlink_tx.c` 是四个工程里差异最大、也最容易改出问题的地方。无论用哪种实现：

1. **非阻塞**：USB 忙的时候不能原地等（尤其不能在中断里等）。
2. **不丢应答**：应答丢一次，上位机就连接失败。队列满时宁可丢"事件"。
3. **缓冲生命周期**：交给 USB 栈的那一帧要活到 `TxReady` 恢复才能复用；
   环里用 in-flight / `tx_prev_buf` 认领，保证同一时刻只有一个上下文在递帧。
4. **上下文安全**：`nexlink_tx_send()` 可能从中断里被调用（adapter 的 UART 中断就会），
   临界区要按上下文选：RTOS 用 `nexlink_lock.h`，裸机用 `__get_PRIMASK()` / `__set_PRIMASK()`。

## 加一个命令

1. **命令号**：`nexlink_proto.h` 的 `NexLinkCmd`（通用段）或 `CMD_USER_BASE + n`（厂商段）。
2. **实现**：
   - 不动协议层 → 在自己的工程里实现 `external_handle_cmd()`（`nexlink_app.c` 里是 `__weak`
     默认实现，只回 `NL_ERR_UNSUPPORTED`）。`nexlink_adapter` 放在 `nexlink_external.c`，
     `nexlink_husb` 放在 `Core/Src/main.c`。
   - 协议层通用命令 → 加 `nexlink_app.c` 里 `handle_cmd()` 的 `switch` case（四个工程要同步）。
3. **回包**：`send_resp_ok(cmd, seq, payload, len)` / `send_resp_err(cmd, seq, err)`；
   主动推状态变化用 `send_event(cmd, payload, len)`（注意它会等上位机连上）。
4. **上位机侧**：`pc-sdk/NexLink/NexLinkDevice.cs` 加对应方法，`NexLink_Tool` 里接入界面。

## 编译与烧录

```bash
# 编译（免开 IDE；-t 指定 target，不写则编当前活动 target）
"C:/Keil_v5/UV4/UV4.exe" -b <工程>.uvprojx -t <target> -j0

# 烧录 / 硬复位
"C:/Program Files (x86)/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe" \
    -c SWD -Q -P <工程>.hex -NoPrompt
```

- 编译结果看 `<输出目录>/<工程>.build_log.htm`（HTML，剥标签后 grep `Error` / `Program Size`）。
- `_app` target 末尾有个 post-build 的 `addheader.bat`，从命令行调 UV4 时可能报
  `CreateProcess failed`（`.bin` 已由前面的 fromelf 生成，只是加头脚本没被拉起来）。
- 用 ST-Link CLI 读内存（不打扰运行）时必须 `-c SWD HOTPLUG`；`-c SWD` 每次都会复位 MCU。

## 各工程备注

- **`nexlink_template`**：内存池 `2 × 1024 + 2 × 64`。心跳在 `StartDefaultTask` 里 1 秒一次，
  并打印三个任务的栈水位（`nexlink_log`）。应用部分是 30×20 小屏示例（Rgb565）。
- **`nexlink_template_bare`**：`Core/Src/freertos.c` 是 CubeMX 残留文件，**没有加入编译**
  （工程里没有 FreeRTOS 源文件）。心跳在主循环里。
- **`nexlink_adapter`**：内存池 `4 × 1024 + 20 × 64`。UART 数据事件是在 **UART 空闲/DMA 中断**
  里直接 `send_event()` 的，所以这条链上的临界区必须中断安全。心跳在 `StartDefaultTask` 里。
- **`nexlink_husb`**：两块板子两个 Keil 工程（f405 / f407），各自 base + `_app`。外设最多：
  NV3030B LCD + EasyUI/LVGL + MPU6050/iMPU + BMP280 + RX8900 RTC + 蜂鸣器 + stmflash，
  另外开了 IWDG（约 0.5s）。自定义命令在 `Core/Src/main.c` 的 `external_handle_cmd()`。
  主循环**没有**调 `nex_send_heartbeat()`。
  - 目前能编过的是 **f405** 工程（`nexlink_f405_husb.uvprojx`）。
  - **`nexlink_f407_husb.uvprojx` 编不过**：它的文件列表是从 `nexlink_template` 抄的，
    引用了 husb 里不存在的 `nexlink_rampool.c`（`nexlink_app.c` / `nexlink_tx.c` 也重复列了），
    并且 include 路径里缺 `ThirdPart/beep/`（`beep.h` 实际在 `ThirdPart/beep/beep.h`）。
  - `MDK-ARM/startup_stm32f407xx.s` 的 `Stack_Size` 只有 `0x400`（1KB），而 USB 中断里的
    `send_resp_internal()` 有个 `tx_buf[1280]` 栈数组 → f407 跑起来会爆栈（f405 是 `0x1000`）。
  - `handle_frame_data()`（`nexlink_app.c`）在中断里 `while(hspi1.State != HAL_SPI_STATE_READY)`：
    主循环正在用 polling 方式写 SPI（`spi_write_8bit()`）时，这个状态位只有被抢占的线程才会清，
    中断里会死等（→ IWDG 复位）；而且和主循环抢同一个 SPI 会打断 "SetRegion + 分片写" 序列导致花屏。
    推荐把 `CMD_FRAME_DATA` 改成中断里只入队、`memcpy` + `NV3030B_DMA_Transfer` 由主循环做。
