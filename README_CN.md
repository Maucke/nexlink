🌐 语言： [English](README.md) | **简体中文**
---

NexLink 是一个轻量级、可扩展的 USB 通信协议及 SDK，专为 PC 与 MCU 之间的可靠通信而设计。

它特别适用于 USB 控制、配置以及高速数据交换场景，例如 USB 转 I2C / SPI 桥接设备、显示控制、调试工具等。

USB 回环（1 KB）在不同平台下的实测性能：

➡️ [USB Loopback Performance Test](./docs/USB_Loopback_Performance_Test.md)

---

NexLink 的设计目标是：**清晰、确定、可长期维护**。

---

主要特性

* 基于 USB Bulk 传输（可靠、高吞吐）
* 双向协议模型：CMD / RESP / EVENT
* 16 位命令空间，具备长期扩展能力
* 基于序列号的请求-应答匹配机制
* MCU 主动上报异步事件
* 适配 RTOS 的下位机实现
* PC 端 SDK（C / C#）
* 面向 STM32 等 MCU 平台设计

---

系统架构

PC

* NexLink PC SDK（C / C#）
* 发送 CMD
* 等待 RESP
* 接收并处理 EVENT

USB Bulk 传输通道

MCU

* NexLink 协议核心
* CMD 处理器
* RESP 发送
* EVENT 主动上报

---

通信模型

CMD
方向：PC → MCU
说明：命令请求

RESP
方向：MCU → PC
说明：对应 CMD 的响应

EVENT
方向：MCU → PC
说明：异步事件，不与 CMD 配对

CMD 与 RESP 通过序列号进行一一匹配。
EVENT 独立存在，不会阻塞命令处理流程。

---

数据包格式

所有数据包均使用统一的二进制结构。

偏移 | 长度 | 字段
0 | 1 | magic（固定标识）
1 | 1 | type（CMD / RESP / EVENT）
2 | 2 | cmd（16 位命令 ID）
4 | 2 | seq（序列号）
6 | 2 | length（payload 长度）
8 | N | payload

说明

* seq 仅用于 CMD / RESP
* EVENT 的 seq 固定为 0
* payload 可以为空
* RESP 的 payload 第一个字节为 errorCode

---

命令 ID 设计（16 位）

命令 ID 采用分域设计：

高 8 位：功能域
低 8 位：子命令

这种设计便于模块化管理，并支持长期扩展。

---

命令 / 事件分配示例

核心 / 系统（0x0000 – 0x00FF）

CMD_PING        0x0001
CMD_GET_VERSION 0x0002
CMD_SYNC_TIME   0x0003

---

日志 / 调试（0x0100 – 0x01FF）

EVT_LOG   0x0100
EVT_WARN  0x0101
EVT_ERROR 0x0102

---

状态 / 监控（0x0200 – 0x02FF）

EVT_STATUS     0x0200
EVT_HEARTBEAT  0x0201

---

外设控制（0x0300 – 0x03FF）

CMD_GPIO_WRITE 0x0300
CMD_I2C_XFER   0x0301
CMD_SPI_XFER   0x0302

---

高速数据 / 帧传输（0x0400 – 0x04FF）

CMD_FRAME_START 0x0400
EVT_FRAME_BEGIN 0x0401
EVT_FRAME_DATA  0x0402
EVT_FRAME_END   0x0403

---

时间基准

MCU 提供单调递增的毫秒计时函数：

uint64_t mcu_time_ms(void);

特性

* 单调递增，不回退
* 通常基于 RTOS Tick
* 用于时间同步、超时和时间戳

PC 端仅计算时间偏移，不影响 MCU 内部时间。

---

MCU 端设计原则

* USB 接收在独立任务 / 线程中完成
* 协议层负责数据包重组
* CMD 处理按包同步执行
* EVENT 发送为非阻塞
* USB 中断中不做复杂逻辑
* 完全兼容 FreeRTOS

---

PC SDK 设计说明

C SDK

* 基于 libusb
* 提供阻塞式 nexlink_cmd() 接口
* 内部接收线程
* 基于序列号的等待队列机制
* 支持 EVENT 回调注册

C# SDK

* 基于 C SDK 的 P/Invoke 封装
* 托管层不解析协议
* 事件驱动 API
* 适合 GUI / 工具类应用

---

可靠性与异常处理

* USB Bulk 保障传输可靠性
* 协议层处理分包与粘包
* 序列号防止响应错配
* USB 断开自动释放等待中的命令
* EVENT 不影响 CMD / RESP 流程

---

典型应用场景

* USB 硬件适配器
* 嵌入式调试控制台
* USB 显示 / 传感器控制
* 设备配置与烧录工具
* 高速遥测与数据流传输

---

许可证

MIT License

---

设计理念

NexLink 不只是一个 USB 协议。
它是 PC 与嵌入式系统之间的一套**确定性、可扩展通信基础设施**。

---