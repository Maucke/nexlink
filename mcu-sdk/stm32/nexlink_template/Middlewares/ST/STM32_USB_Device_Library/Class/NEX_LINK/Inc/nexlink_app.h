#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nexlink_proto.h"

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

void nexlink_log(const char *fmt, ...);

/**
 * @brief   上位机是否已经连上
 *
 * 收到第一条合法 CMD 才置位（PC 的 NexLink_Tool 连接时会先发 CMD_GET_VERSION），
 * USB 复位/去配置时清零。只有置位后才允许主动往 PC 推数据（心跳、log 等）：
 * PC 侧没在读的时候，EP1 IN 上挂着的包不会被读走，TxReady 一直是 false，
 * 之后再发任何东西都会被丢掉。
 */
bool nexlink_host_connected(void);

void nexlink_host_reset(void);

void nex_send_heartbeat(void);

void send_resp_err(
    uint16_t cmd,
    uint16_t seq,
    nl_err_t err);

void send_resp_ok(
    uint16_t cmd,
    uint16_t seq,
    const void *payload,
    uint16_t len);

void send_event(
    uint16_t cmd,
    const void *payload,
    uint16_t len);
		
void upload_frame_upload(void);
	
#ifdef __cplusplus
}
#endif
