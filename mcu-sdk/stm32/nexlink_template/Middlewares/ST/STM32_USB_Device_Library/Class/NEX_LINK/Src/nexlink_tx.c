#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "usbd_nex_link.h"
#include "nexlink_rampool.h"
#include "nexlink_lock.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/*
 * 发送队列 = 指向内存池缓冲的环形队列（零拷贝：调用方直接在池缓冲里组包）。
 *
 * 为什么不直接用 FreeRTOS 队列：
 *   - 任务侧原来是 xQueueSend(portMAX_DELAY)，主机不读时会把回命令的 RX 任务
 *     一起堵死；中断侧 xQueueSendFromISR 队列满就静默丢帧，而且丢的那帧缓冲
 *     永远没人 free，泄漏几次内存池就空了。
 *   - 现在改成非阻塞入队：满了就丢这一帧（谁入队谁 buf_free），TX 路径永远能
 *     自我恢复；"别在没连 PC 之前发"由 nexlink_app.c 的连接闸门保证。
 *
 * 缓冲所有权：buf_alloc() -> nexlink_tx_send() -> nexlink_tx_poll() -> usb_tx()，
 * 最后在 nexlink_tx_poll() 里观察到 TxReady 之后 buf_free()。
 *
 * nexlink_tx_send() 会立刻泵一次队列：应答不能等主循环/等另一个任务调度，
 * 否则主机第一条命令就可能超时。因此 nexlink_tx_poll() 可能被 RX 任务、
 * 中断和 TX 任务同时调用，用 tx_prev_buf 认领来保证同一时刻只有一个上下文
 * 在往 USB 栈里递帧。
 */

#define TX_SLOT_COUNT (BUF_COUNT + SMALL_BUF_COUNT)

static uint8_t *tx_bufs[TX_SLOT_COUNT];
static uint16_t tx_lens[TX_SLOT_COUNT];
static volatile uint8_t tx_head;        /* 生产者写 */
static volatile uint8_t tx_tail;        /* 消费者写 */

/* 已交给 USB 栈、还没传完的那一帧（同时是"有人在发"的认领标志） */
static uint8_t *tx_prev_buf;

extern USBD_HandleTypeDef hUSB;

#define TX_NEXT(i) ((uint8_t)(((i) + 1U) % TX_SLOT_COUNT))

bool nexlink_tx_send(const void *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U))
        return false;

    bool queued = false;

    uint32_t lock = nexlink_lock_enter();

    uint8_t next = TX_NEXT(tx_head);

    if (next != tx_tail)
    {
        /* 写在 tx_head 处再推进 tx_head 来发布：消费者读的是 tx_bufs[tx_tail]，
           写 TX_NEXT(tx_head) 会让消费者永远慢一帧 */
        tx_bufs[tx_head] = (uint8_t *)buf;
        tx_lens[tx_head] = len;
        tx_head = next;
        queued = true;
    }

    nexlink_lock_exit(lock);

    if (!queued)
        buf_free((uint8_t *)buf);          /* 队列满：谁入队谁负责回收 */

    nexlink_tx_poll();                     /* 非阻塞：USB 空闲就当场发出去 */

    return queued;
}

void nexlink_tx_poll(void)
{
    uint8_t *free_buf = NULL;
    uint8_t *send_buf = NULL;
    uint16_t len = 0;

    uint32_t lock = nexlink_lock_enter();

    if ((tx_prev_buf != NULL) && USBD_NEX_LINK_TxReady(&hUSB))
    {
        free_buf = tx_prev_buf;            /* 上一帧主机读走了，缓冲可以归还 */
        tx_prev_buf = NULL;
    }

    if ((tx_prev_buf == NULL) && (tx_head != tx_tail) && USBD_NEX_LINK_TxReady(&hUSB))
    {
        send_buf = tx_bufs[tx_tail];
        len = tx_lens[tx_tail];
        tx_tail = TX_NEXT(tx_tail);
        tx_prev_buf = send_buf;            /* 认领：其它上下文不能再递帧 */
    }

    nexlink_lock_exit(lock);

    if (free_buf != NULL)
        buf_free(free_buf);

    if (send_buf != NULL)
        usb_tx(send_buf, len);
}

void nexlink_tx_reset(void)
{
    uint32_t lock = nexlink_lock_enter();

    while (tx_tail != tx_head)
    {
        uint8_t idx = tx_tail;
        tx_tail = TX_NEXT(idx);
        buf_free(tx_bufs[idx]);
    }

    nexlink_lock_exit(lock);
    /* tx_prev_buf 在 USB 栈手里，等 nexlink_tx_poll() 回收 */
}

void NexLinkTxTask(void *arg)
{
    (void)arg;

    for (;;)
    {
        nexlink_tx_poll();

        /* 空转时让出 CPU；有数据时最差一个 tick 的延迟 */
        osDelay(1);
    }
}
