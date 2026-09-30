# NexLink

NexLink 是一个基于 **USB Vendor (WinUSB/libusb)** 的轻量级通信协议与运行时框架，
用于 ESP32 / STM32 等 MCU 与 PC 主机之间进行 **高可靠、低延迟、可扩展** 的数据交互。

该组件基于 **TinyUSB + FreeRTOS** 实现，支持：

- 请求 / 响应（CMD / RESP）
- 设备 → 主机异步事件（EVENT）
- 事件闸门：收到第一条合法 CMD（上位机已连上）之后才允许主动推事件
- 可移植协议层（与 USB / UART / SPI 解耦）

---

## 特性

- 🚀 USB Vendor 通道（WinUSB / libusb）
- 🔁 全双工通信
- 📦 自定义二进制协议（小端、零拷贝友好）
- 🧵 FreeRTOS 安全（Queue + Task 解耦）
- 🔌 Host Ready 机制（防止 USB stall / dead）
- 🧱 可独立作为 ESP-IDF component 使用

---

## USB 配置与吞吐（重要）

- **TinyUSB 的配置来自 `esp_tinyusb` 组件，不是本组件。**
  TinyUSB 各源文件（`tusb.c` / `class/vendor/vendor_device.c`）的编译命令里只有
  `managed_components/espressif__esp_tinyusb/include`，看不到本组件的 `include/`，
  所以本目录下**不要**再放 `tusb_config.h`（放了一份也不会生效，反而会让应用侧和
  协议栈侧看到两套不一致的 `CFG_TUD_*`）。
- 实际生效的关键配置（`espressif__esp_tinyusb/include/tusb_config.h`）：

  | 宏 | 值 |
  |---|---|
  | `CFG_TUD_VENDOR` | `CONFIG_TINYUSB_VENDOR_COUNT` = 1 |
  | `CFG_TUD_VENDOR_RX/TX_BUFSIZE` | `TUD_OPT_HIGH_SPEED ? 512 : 64` → **ESP32-S3 上是 64 字节** |
  | `CFG_TUD_CDC_RX/TX_BUFSIZE` | `CONFIG_TINYUSB_CDC_*_BUFSIZE`（在 `sdkconfig` 里配） |

- **多任务用 TinyUSB 的规矩**：`tud_task()` 只能有一个驱动者。本工程用的是
  esp_tinyusb 自带的任务（`CONFIG_TINYUSB_NO_DEFAULT_TASK` 未置位），所以应用侧
  **不要**再调 `tud_task()`；应用任务只做 `tud_vendor_write()/flush()` 和读，
  写 FIFO 满了就等下一次（`nexlink_tx_poll()` 就是这个语义）。
- **吞吐**：ESP32-S3 的 USB-OTG 只有 Full Speed，vendor bulk 实际上限约
  **64 KB/s**（每 1 ms 帧一个 64 字节包），1 KB 的帧要 ~16 ms。同一个协议在
  STM32 那四个工程上是 USB HS + ULPI（480 Mbps），所以做图像 / 大块数据上传时
  ESP32 侧会是瓶颈，需要压缩、降帧或换通道。


---