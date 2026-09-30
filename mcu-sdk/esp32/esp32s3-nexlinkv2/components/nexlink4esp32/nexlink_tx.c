#include "nexlink_tx.h"
#include "nexlink_app.h"
#include "nexlink_tasks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"

/*
 * 发送队列 + 一个"正在发"的帧。
 *
 * 三个关键点（都是踩过的）：
 *
 * 1. 不能在这里调 tud_task()。TinyUSB 由 esp_tinyusb 自带的那个任务驱动
 *    （CONFIG_TINYUSB_NO_DEFAULT_TASK 没置位），两个任务同时跑 tud_task()
 *    会并发访问设备栈内部状态。本文件只用 tud_vendor_write()/flush()。
 *
 * 2. 不能阻塞等待。vendor FIFO 实际只有 64 字节（见 README：生效的是 esp_tinyusb
 *    的 tusb_config.h），主机不读时很快满。写不下就返回、下次续写，不占住 RX。
 *
 * 3. 应答不能被丢掉。每帧都占一块池缓冲，主机不读（比如上位机断开但没复位设备）
 *    时事件会把队列和池子占满，后面的命令应答就拿不到缓冲 —— 上位机超时，表现得
 *    就像"连不上"。所以：应答（droppable=false）优先，必要时先丢事件腾地方；
 *    并且发现主机长时间不读就先把事件闸门关上（nexlink_event_disable()），
 *    别再往池子里堆。闸门会在下一条命令（上位机重连时必发）时自动重新打开。
 */

static QueueHandle_t txq;

static uint8_t tx_buf_pool[TX_BUF_COUNT][TX_BUF_SIZE];
static uint8_t tx_buf_used[TX_BUF_COUNT] = {0};

/* alloc/free 可能来自不同任务（log_task 发日志、NexLinkTask 发完回收），
   而两者可能跑在不同核上，必须互斥 */
static portMUX_TYPE tx_pool_mux = portMUX_INITIALIZER_UNLOCKED;

static void tx_purge_events(void)
{
    tx_item_t kept[TX_BUF_COUNT];
    int kept_n = 0;

    tx_item_t item;
    while (xQueueReceive(txq, &item, 0) == pdTRUE)
    {
        if (item.droppable)
            tx_buf_free(item.buf);
        else if (kept_n < TX_BUF_COUNT)
            kept[kept_n++] = item;
    }

    for (int i = 0; i < kept_n; i++)
        xQueueSend(txq, &kept[i], 0);
}

void nexlink_tx_drop_events(void)
{
    tx_purge_events();
}

void nexlink_tx_init(void)
{
    txq = xQueueCreate(TX_BUF_COUNT, sizeof(tx_item_t));
}

uint8_t *tx_buf_alloc(void)
{
    uint8_t *ret = NULL;

    portENTER_CRITICAL(&tx_pool_mux);
    for (int i = 0; i < TX_BUF_COUNT; i++)
    {
        if (!tx_buf_used[i])
        {
            tx_buf_used[i] = 1;
            ret = tx_buf_pool[i];
            break;
        }
    }
    portEXIT_CRITICAL(&tx_pool_mux);

    return ret;
}

void tx_buf_free(uint8_t *buf)
{
    if (buf == NULL)
        return;

    portENTER_CRITICAL(&tx_pool_mux);
    for (int i = 0; i < TX_BUF_COUNT; i++)
    {
        if (tx_buf_pool[i] == buf)
        {
            tx_buf_used[i] = 0;
            break;
        }
    }
    portEXIT_CRITICAL(&tx_pool_mux);
}

static void tx_post(const void *buf, uint16_t len, bool droppable)
{
    tx_item_t item;

    if ((buf == NULL) || (len == 0) || (len > TX_BUF_SIZE))
    {
        tx_buf_free((uint8_t *)buf);
        return;
    }

    item.len = len;
    item.buf = (uint8_t *)buf;
    item.droppable = droppable;

    bool queued = (xQueueSend(txq, &item, 0) == pdTRUE);

    if (!queued && !droppable)
    {
        tx_purge_events();                                       /* 应答优先：先丢事件 */
        queued = (xQueueSend(txq, &item, 0) == pdTRUE);
    }

    if (!queued)
    {
        tx_buf_free(item.buf);                                   /* 事件：丢掉，缓冲必须回收 */
        return;
    }

    /* 叫醒 NexLinkTask 尽快发出去（否则要等它的兜底超时） */
    if (nexlink_task_handle)
        xTaskNotifyGive(nexlink_task_handle);
}

void nexlink_tx_send_resp(const void *buf, uint16_t len)
{
    tx_post(buf, len, false);
}

void nexlink_tx_send_event(const void *buf, uint16_t len)
{
    tx_post(buf, len, true);
}

void nexlink_tx_poll(void)
{
    static tx_item_t current;
    static uint32_t sent;
    static bool busy;
    static TickType_t last_progress;

    if (!busy)
    {
        if (xQueueReceive(txq, &current, 0) != pdTRUE)
            return;

        sent = 0;
        busy = true;
        last_progress = xTaskGetTickCount();
    }

    /* 返回实际写进 FIFO 的字节数，FIFO 满时为 0（不阻塞） */
    uint32_t n = tud_vendor_write(current.buf + sent, (uint32_t)(current.len - sent));

    if (n)
    {
        sent += n;
        last_progress = xTaskGetTickCount();
    }

    if (sent < current.len)
    {
        /* 半分钟都写不进一个字节：主机没在读（上位机断开但设备没复位就是这种）。
           先把事件闸门关上，别让 log/事件继续把池子占满，否则下一条命令的应答
           分配不到缓冲。上位机重连后发来的第一条命令会重新打开闸门。 */
        if ((xTaskGetTickCount() - last_progress) > pdMS_TO_TICKS(500))
            nexlink_event_disable();

        return;
    }

    /* 整帧写完才 flush 提交，避免把一帧拆成多次 transfer 提交给主机 */
    tud_vendor_flush();
    tx_buf_free(current.buf);
    busy = false;
}
