#pragma once
#include <stdint.h>
#include <stdbool.h>

#define TX_BUF_SIZE   1024    // 每个 TX buffer 的大小
#define TX_BUF_COUNT  4       // TX buffer 数量

/*
 * 队列里每一项都占着一块池缓冲，所以：
 *   - 应答（RESP）不可丢：丢了上位机就超时，表现为"连不上"；
 *   - 事件（EVENT）可丢：队列/池满时，先丢它们给应答腾地方。
 * droppable 就是标记。
 */
typedef struct
{
    uint16_t len;
    uint8_t *buf;
    bool droppable;
} tx_item_t;

void nexlink_tx_init(void);

/* buf 必须是 tx_buf_alloc() 拿到的缓冲，所有权随之移交。
   两者都是非阻塞的：入队失败时由这两个函数负责 tx_buf_free()。 */
void nexlink_tx_send_resp(const void *buf, uint16_t len);
void nexlink_tx_send_event(const void *buf, uint16_t len);

/* 丢掉队列里还没发出去的可丢帧（给应答腾池缓冲/队列位置） */
void nexlink_tx_drop_events(void);

/*
 * 把队列里的帧写进 TinyUSB 的 vendor TX FIFO。非阻塞：
 * FIFO 满就返回，剩下的字节下次续写；不完成当前帧就不会取下一帧。
 * 由 NexLinkTask 在 USB 事件唤醒后调用。
 */
void nexlink_tx_poll(void);

uint8_t *tx_buf_alloc(void);
void tx_buf_free(uint8_t *buf);
