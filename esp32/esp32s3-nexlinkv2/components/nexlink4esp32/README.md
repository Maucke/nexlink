# NexLink

NexLink 是一个基于 **USB Vendor (WinUSB/libusb)** 的轻量级通信协议与运行时框架，
用于 ESP32 / STM32 等 MCU 与 PC 主机之间进行 **高可靠、低延迟、可扩展** 的数据交互。

该组件基于 **TinyUSB + FreeRTOS** 实现，支持：

- 请求 / 响应（CMD / RESP）
- 设备 → 主机异步事件（EVENT）
- 主机控制设备主动上传（Event Enable / Mask）
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