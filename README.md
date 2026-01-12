---

NexLink is a lightweight, extensible USB communication protocol and SDK designed for reliable PC ↔ MCU communication.

It is optimized for USB-based control, configuration, and high-speed data exchange, especially in USB-to-I2C / SPI bridge scenarios.

---

NexLink focuses on clarity, determinism, and long-term maintainability.

---

FEATURES

* USB Bulk transfer (reliable, high throughput)
* Bidirectional protocol: CMD / RESP / EVENT
* 16-bit command space (future-proof)
* Sequence-based request/response matching
* MCU-initiated asynchronous events
* RTOS-friendly MCU implementation
* PC SDK (C / C#)
* Designed for STM32-class MCUs

---

SYSTEM ARCHITECTURE

PC

* NexLink PC SDK (C / C#)
* Send CMD
* Wait RESP
* Handle EVENT

USB Bulk Transport

MCU

* NexLink protocol core
* CMD handler
* RESP sender
* EVENT sender

---

COMMUNICATION MODEL

CMD
Direction: PC → MCU
Description: Request command

RESP
Direction: MCU → PC
Description: Response to CMD

EVENT
Direction: MCU → PC
Description: Asynchronous, unpaired message

CMD and RESP are matched using a sequence number.
EVENT packets are independent and never block command processing.

---

PACKET FORMAT

All packets share the same binary layout.

Offset | Size | Field
0 | 1 | magic (fixed value)
1 | 1 | type (CMD / RESP / EVENT)
2 | 2 | cmd (16-bit command ID)
4 | 2 | seq (sequence number)
6 | 2 | length (payload length)
8 | N | payload

Notes

* seq is used only for CMD / RESP
* EVENT packets always use seq = 0
* payload may be empty

---

COMMAND ID DESIGN (16-BIT)

Command IDs are domain-based:

High 8 bits: domain
Low 8 bits: sub-command

This structure allows clean grouping and future expansion.

---

COMMAND / EVENT ALLOCATION

Core / System (0x0000 – 0x00FF)

CMD_PING        0x0001
CMD_GET_VERSION 0x0002
CMD_SYNC_TIME   0x0003

---

Log / Debug (0x0100 – 0x01FF)

EVT_LOG   0x0100
EVT_WARN  0x0101
EVT_ERROR 0x0102

---

Status / Monitor (0x0200 – 0x02FF)

EVT_STATUS     0x0200
EVT_HEARTBEAT  0x0201

---

Peripheral Control (0x0300 – 0x03FF)

CMD_GPIO_WRITE 0x0300
CMD_I2C_XFER   0x0301
CMD_SPI_XFER   0x0302

---

High-Speed / Frame Data (0x0400 – 0x04FF)

CMD_FRAME_START 0x0400
EVT_FRAME_BEGIN 0x0401
EVT_FRAME_DATA  0x0402
EVT_FRAME_END   0x0403

---

TIME BASE

MCU provides a monotonic millisecond counter:

uint64_t mcu_time_ms(void);

Characteristics

* Monotonic, never adjusted
* Usually derived from RTOS tick
* Used for time sync, timeouts, timestamps

PC computes offsets logically; MCU time remains absolute.

---

MCU DESIGN PRINCIPLES

* USB RX handled in a dedicated task/thread
* Packet reassembly handled at protocol layer
* Command handling is synchronous per packet
* EVENT sending is non-blocking
* No heavy logic inside USB ISR
* Fully compatible with FreeRTOS

---

PC SDK DESIGN

C SDK

* libusb-based backend
* Blocking nexlink_cmd() with timeout
* Internal RX thread
* Sequence-based waiter mechanism
* EVENT callback registration

C# SDK

* P/Invoke wrapper over C SDK
* No protocol parsing in managed code
* Event-driven API
* Suitable for GUI applications

---

RELIABILITY AND ERROR HANDLING

* USB Bulk ensures data integrity
* Application layer handles packet fragmentation and coalescing
* Sequence matching prevents mis-routing
* USB disconnect releases pending waiters
* EVENT delivery never blocks CMD/RESP

---

TYPICAL USE CASES

* USB hardware adapters
* Embedded debug consoles
* USB display or sensor controllers
* Device provisioning tools
* High-speed telemetry streaming

---

LICENSE

MIT License

---

DESIGN PHILOSOPHY

NexLink is not just a USB protocol.
It is a deterministic, extensible communication fabric between PC and embedded systems.

---
