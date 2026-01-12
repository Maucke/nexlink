#include "nexlink_app.h"
#include "nexlink_proto.h"
#include "nexlink_tx.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>

#define NEXLINK_LOG_MAX_LEN   96   

static uint8_t rx_buf[512];
static uint16_t rx_len;

uint64_t mcu_time_ms(void)
{
    TickType_t tick = xTaskGetTickCount();
    return (uint64_t)tick * (1000 / configTICK_RATE_HZ);
}

static void send_resp(uint16_t cmd, uint16_t seq,
                      const void *payload, uint16_t len)
{
    uint8_t tx[128];
    nl_packet_t *pkt = (nl_packet_t *)tx;

    pkt->magic  = NL_MAGIC;
    pkt->type   = NL_PKT_RESP;
    pkt->cmd    = cmd;
    pkt->seq    = seq;
    pkt->length = len;

    if (len) memcpy(pkt->payload, payload, len);
    nexlink_tx_send(pkt, HEAD_LEN + len);
}

static void send_event(uint16_t cmd,
                       const void *payload, uint16_t len)
{
    uint8_t tx[128];
    nl_packet_t *pkt = (nl_packet_t *)tx;

    pkt->magic  = NL_MAGIC;
    pkt->type   = NL_PKT_EVENT;
    pkt->cmd    = cmd;
    pkt->seq    = 0;       
    pkt->length = len;

    if (len)
        memcpy(pkt->payload, payload, len);

    nexlink_tx_send(pkt, HEAD_LEN + len);
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

static void handle_cmd(nl_packet_t *pkt)
{
    switch (pkt->cmd)
    {
    case CMD_PING:
        send_resp(pkt->cmd, pkt->seq, NULL, 0);
        break;

    case CMD_SYNC_TIME:
    {
        uint64_t t = mcu_time_ms();
        send_resp(pkt->cmd, pkt->seq, &t, sizeof(t));
        break;
    }
    }
}

void nexlink_rx_bytes(const uint8_t *data, uint16_t len)
{
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
        uint16_t total = HEAD_LEN + plen;
        if (rx_len < total) return;

        nl_packet_t *pkt = (nl_packet_t *)rx_buf;
        if (pkt->type == NL_PKT_CMD)
            handle_cmd(pkt);

        memmove(rx_buf, rx_buf + total, rx_len - total);
        rx_len -= total;
    }
}
