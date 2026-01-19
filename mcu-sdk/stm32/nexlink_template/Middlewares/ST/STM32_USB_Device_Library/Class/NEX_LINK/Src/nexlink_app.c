#include "nexlink_app.h"
#include "nexlink_proto.h"
#include "nexlink_rampool.h"
#include "nexlink_tasks.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>

#define NEXLINK_LOG_MAX_LEN 96

uint64_t mcu_time_ms(void)
{
    TickType_t tick = xTaskGetTickCount();
    return (uint64_t)tick * (1000 / configTICK_RATE_HZ);
}

static void send_resp(uint16_t cmd, uint16_t seq,
                      const void *payload, uint16_t len)
{
    uint16_t total_len = HEAD_LEN + len;
    uint8_t *tx = buf_alloc();
    if (!tx || total_len > BUF_SIZE)
        return;

    nl_packet_t *pkt = (nl_packet_t *)tx;
    pkt->magic = NL_MAGIC;
    pkt->type = NL_PKT_RESP;
    pkt->cmd = cmd;
    pkt->seq = seq;
    pkt->length = len;

    if (len)
        memcpy(pkt->payload, payload, len);

    nexlink_send(tx, total_len);
}

static void send_resp_ok(
    uint16_t cmd, uint16_t seq,
    const void *payload, uint16_t len)
{
    uint8_t buf[1 + len];
    buf[0] = NL_ERR_OK;

    if (len)
        memcpy(buf + 1, payload, len);

    send_resp(cmd, seq, buf, sizeof(buf));
}

static void send_resp_err(uint16_t cmd, uint16_t seq, nl_err_t err)
{
    uint8_t e = err;
    send_resp(cmd, seq, &e, 1);
}

static void send_event(uint16_t cmd,
                       const void *payload, uint16_t len)
{
    uint16_t total_len = HEAD_LEN + len;
    uint8_t *tx = buf_alloc();
    if (!tx || total_len > BUF_SIZE)
        return;

    nl_packet_t *pkt = (nl_packet_t *)tx;
    pkt->magic = NL_MAGIC;
    pkt->type = NL_PKT_EVENT;
    pkt->cmd = cmd;
    pkt->seq = 0;
    pkt->length = len;

    if (len)
        memcpy(pkt->payload, payload, len);

    nexlink_send(tx, HEAD_LEN + len);
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

void nexlink_dispatch(nl_packet_t *pkt)
{
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

bool nexlink_frame_valid(const uint8_t *buf, uint16_t len)
{
    if (len < HEAD_LEN)
        return false;

    const nl_packet_t *hdr = (const nl_packet_t *)buf;

    if (hdr->magic != NL_MAGIC)
        return false;

    switch (hdr->type)
    {
    case NL_PKT_CMD:
    case NL_PKT_RESP:
    case NL_PKT_EVENT:
        break;
    default:
        return false;
    }

    if ((uint32_t)HEAD_LEN + hdr->length != len)
        return false;

    if (hdr->type == NL_PKT_EVENT && hdr->seq != 0)
        return false;

    return true;
}
