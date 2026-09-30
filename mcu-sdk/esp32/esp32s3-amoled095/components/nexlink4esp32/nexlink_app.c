#include "nexlink_app.h"
#include "nexlink_proto.h"
#include "nexlink_tx.h"
#include "tinyusb.h"
#include "tusb.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdarg.h>
#include <stdio.h>

#define NEXLINK_LOG_MAX_LEN 96

/* 一个完整的最大帧 = HEAD_LEN + NL_MAX_PAYLOAD，缓冲必须比它大，
   否则合法的最大帧就会写越界 */
#define RX_BUF_SIZE (NL_MAX_PAYLOAD + HEAD_LEN)

static uint8_t rx_buf[RX_BUF_SIZE];
static uint16_t rx_len;

static volatile bool g_event_enabled = false;

bool nexlink_event_allowed(uint16_t cmd)
{
    if (!g_event_enabled)
        return false;
    return true;
}

void nexlink_event_disable(void)
{
    g_event_enabled = false;
}

/* 掉线（设备被复位/去配置）后不再主动推事件。
   注意：上位机只是关闭句柄、没有复位设备时这个回调不会触发，
   那种情况靠 nexlink_tx.c 里"发送卡住 → nexlink_event_disable()"兜底。 */
void tud_umount_cb(void)
{
    g_event_enabled = false;
}

uint64_t mcu_time_ms(void)
{
    TickType_t tick = xTaskGetTickCount();
    return (uint64_t)tick * (1000 / configTICK_RATE_HZ);
}

/* RESP payload = status(1) + data */
static void send_resp(uint16_t cmd, uint16_t seq, uint8_t status,
                      const void *payload, uint16_t len)
{
    if (len > NL_MAX_PAYLOAD - 1)
        return;

    uint16_t payload_len = 1 + len;
    uint16_t total_len = HEAD_LEN + payload_len;

    /* 先判长度再取缓冲：反过来的话 return 时就把 tx 漏掉了 */
    if (total_len > TX_BUF_SIZE)
        return;

    uint8_t *tx = tx_buf_alloc();

    if (!tx)
    {
        /* 缓冲池被事件占满（主机不读的时候会这样）：丢掉排队的事件给应答腾地方。
           应答一次都不能丢 —— 丢一次上位机就超时，表现得像"连不上" */
        nexlink_tx_drop_events();
        tx = tx_buf_alloc();
    }

    if (!tx)
        return;

    nl_packet_t *pkt = (nl_packet_t *)tx;
    pkt->magic = NL_MAGIC;
    pkt->type = NL_PKT_RESP;
    pkt->cmd = cmd;
    pkt->seq = seq;
    pkt->length = payload_len;

    /* 直接写进 TX 缓冲，别再经过一层栈上的临时数组 */
    pkt->payload[0] = status;
    if (len && payload)
        memcpy(pkt->payload + 1, payload, len);

    nexlink_tx_send_resp(tx, total_len);
}

static void send_resp_ok(uint16_t cmd, uint16_t seq,
                         const void *payload, uint16_t len)
{
    send_resp(cmd, seq, NL_ERR_OK, payload, len);
}

static void send_resp_err(uint16_t cmd, uint16_t seq, nl_err_t err)
{
    send_resp(cmd, seq, (uint8_t)err, NULL, 0);
}

static void send_event(uint16_t cmd,
                       const void *payload, uint16_t len)
{
    if (!nexlink_event_allowed(cmd))
        return;

    uint16_t total_len = HEAD_LEN + len;

    /* 先判长度再取缓冲，避免漏缓冲 */
    if (total_len > TX_BUF_SIZE)
        return;

    uint8_t *tx = tx_buf_alloc();
    if (!tx)
        return;

    nl_packet_t *pkt = (nl_packet_t *)tx;
    pkt->magic = NL_MAGIC;
    pkt->type = NL_PKT_EVENT;
    pkt->cmd = cmd;
    pkt->seq = 0;
    pkt->length = len;

    if (len)
        memcpy(pkt->payload, payload, len);

    nexlink_tx_send_event(tx, total_len);
}

void nexlink_log(const char *fmt, ...)
{
    static char buf[NEXLINK_LOG_MAX_LEN];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n <= 0)
        return;

    if (n >= sizeof(buf))
        n = sizeof(buf) - 1;

    send_event(EVT_LOG, buf, (uint16_t)n);
}

static void nexlink_cmd_loopback(const nl_packet_t *req)
{
    nl_err_t err = NL_ERR_OK;

    if (req->length > NL_MAX_PAYLOAD)
    {
        err = NL_ERR_INVALID_PARAM;
        send_resp_err(req->cmd, req->seq, err);
        return;
    }

   send_resp_ok(req->cmd, req->seq,
                 req->payload, req->length);
}

static void handle_cmd(nl_packet_t *pkt)
{
    g_event_enabled = true;
    switch (pkt->cmd)
    {
    case CMD_PING:
        send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
        break;

    case CMD_LOOPBACK:
        nexlink_cmd_loopback(pkt);
        break;

    case CMD_SYNC_TIME:
    {
        uint64_t t = mcu_time_ms();
        send_resp_ok(pkt->cmd, pkt->seq, &t, sizeof(t));
        break;
    }
    case CMD_GET_VERSION:
    {
        nl_version_t ver = {
            .major = NL_VERSION_MAJOR,
            .minor = NL_VERSION_MINOR,
            .patch = NL_VERSION_PATCH,
            .build = NL_VERSION_BUILD,
        };

        send_resp_ok(pkt->cmd, pkt->seq, &ver, sizeof(ver));
        break;
    }
    default:
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_UNSUPPORTED);
        break;
    }
}

void nexlink_rx_bytes(const uint8_t *data, uint16_t len)
{
    /* 先保证装得下再 memcpy。帧长上限就是 RX_BUF_SIZE，真装不下说明流已经错位，
       丢掉重来（只保留最后一段，避免整块丢弃后永远对不上） */
    if (len > (uint16_t)(RX_BUF_SIZE - rx_len))
    {
        rx_len = 0;

        if (len > RX_BUF_SIZE)
        {
            data += (len - RX_BUF_SIZE);
            len = RX_BUF_SIZE;
        }
    }

    memcpy(rx_buf + rx_len, data, len);
    rx_len += len;

    while (rx_len >= HEAD_LEN)
    {
        if (rx_buf[0] != NL_MAGIC)
        {
            memmove(rx_buf, rx_buf + 1, --rx_len);
            continue;
        }

        uint16_t plen = *(uint16_t *)(rx_buf + 6);

        /* length 字段来自报文，必须校验：否则这里会一直等一个永远凑不齐的帧，
           而新的字节还在不断被 memcpy 进缓冲 → 越界写 */
        if (plen > NL_MAX_PAYLOAD)
        {
            memmove(rx_buf, rx_buf + 1, --rx_len);
            continue;
        }

        uint16_t total = HEAD_LEN + plen;
        if (rx_len < total)
            return;

        nl_packet_t *pkt = (nl_packet_t *)rx_buf;
        if (pkt->type == NL_PKT_CMD)
            handle_cmd(pkt);

        memmove(rx_buf, rx_buf + total, rx_len - total);
        rx_len -= total;
    }
}
