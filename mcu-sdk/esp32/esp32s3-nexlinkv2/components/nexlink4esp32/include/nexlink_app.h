#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
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
     * @brief   事件闸门
     *
     * 收到第一条合法命令（上位机已连上）之前、以及主机长时间不读（发送卡住）时，
     * 不允许主动推事件：主机没在读的时候推出去只会挂在 USB 上，把发送队列和
     * 缓冲池占满，最终连命令应答都发不出去。应答（send_resp）不受闸门影响。
     */
    bool nexlink_event_allowed(uint16_t cmd);
    void nexlink_event_disable(void);

#ifdef __cplusplus
}
#endif
