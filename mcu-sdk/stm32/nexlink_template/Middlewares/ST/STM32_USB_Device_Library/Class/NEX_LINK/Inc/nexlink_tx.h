#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
 * 入队一帧（不拷贝，接管 buf 的所有权）。非阻塞，可在任务/中断上下文调用。
 * 队列满时返回 false，并自行 buf_free(buf) —— 调用方不需要管缓冲回收，
 * 也就不会出现"入队失败导致内存池泄漏"。
 */
bool nexlink_tx_send(const void *buf, uint16_t len);

/* 把队头交给 USB 栈（USB 空闲时才发）。由 NexLinkTxTask 循环调用。 */
void nexlink_tx_poll(void);

/* 丢弃队列里还没发出去的帧（正在传输的那一帧属于 USB 栈，不动） */
void nexlink_tx_reset(void);

void NexLinkTxTask(void *arg);
