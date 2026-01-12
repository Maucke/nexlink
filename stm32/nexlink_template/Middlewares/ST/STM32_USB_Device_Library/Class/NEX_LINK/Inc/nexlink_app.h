#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   协议层入口：喂入从 USB/RingBuf 拿到的原始字节流
 *
 * @param data  字节数据指针
 * @param len   数据长度
 *
 * 调用场景：
 *   - NexLinkRxTask 中
 *   - 从 ringbuf_read() 取出数据后调用
 *
 * 特点：
 *   - 支持粘包 / 拆包
 *   - 内部维护解析状态
 *   - 自动分发 CMD
 */
void nexlink_rx_bytes(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
