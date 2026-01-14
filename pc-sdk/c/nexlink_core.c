#include "nexlink_internal.h"
#include "nexlink_usb.h"
#include <stdlib.h>
#include <string.h>
#include "nexlink_core.h"

/* ================= RX PARSER ================= */

static void dispatch_packet(
    nl_ctx_t* ctx,
    const nexlink_packet_t* pkt)
{
    nexlink_event_cb cb = NULL;
    void* user = NULL;

    pthread_mutex_lock(&ctx->lock);

    if (pkt->type == NL_PKT_RESP)
    {
        nl_waiter_t* w = ctx->waiters;
        while (w)
        {
            if (w->seq == pkt->seq)
            {
                memcpy(&w->resp, pkt, sizeof(*pkt));
                w->done = 1;
                pthread_cond_signal(&w->cond);
                break;
            }
            w = w->next;
        }
    }
    else if (pkt->type == NL_PKT_EVENT)
    {
        cb = ctx->event_cb;
        user = ctx->event_user;
    }

    pthread_mutex_unlock(&ctx->lock);

    /* ⚠️ 回调必须在锁外 */
    if (cb)
        cb(user, pkt);
}

static void parse_rx(
    nl_ctx_t* ctx,
    const uint8_t* data,
    int len)
{
    memcpy(ctx->rx_buf + ctx->rx_len, data, len);
    ctx->rx_len += len;

    while (ctx->rx_len >= HEAD_LEN)
    {
        if (ctx->rx_buf[0] != NL_MAGIC)
        {
            memmove(ctx->rx_buf,
                ctx->rx_buf + 1,
                --ctx->rx_len);
            continue;
        }

        uint16_t plen =
            *(uint16_t*)(ctx->rx_buf + 6);

        uint16_t total = HEAD_LEN + plen;
        if (ctx->rx_len < total)
            return;

        nexlink_packet_t pkt;
        memcpy(&pkt, ctx->rx_buf, total);

        memmove(ctx->rx_buf,
            ctx->rx_buf + total,
            ctx->rx_len - total);
        ctx->rx_len -= total;

        dispatch_packet(ctx, &pkt);
    }
}

/* ================= RX THREAD ================= */

static void* rx_thread_fn(void* arg)
{
    nl_ctx_t* ctx = arg;
    uint8_t buf[512];

    while (ctx->running)
    {
        int n = usb_bulk_read(
            ctx->usb,
            buf,
            sizeof(buf),
            1000);

        //if (n < 0)
        //    break;

        if (n > 0)
            parse_rx(ctx, buf, n);
    }

    /* USB 异常 / 退出，唤醒所有 waiter */
    pthread_mutex_lock(&ctx->lock);

    nl_waiter_t* w = ctx->waiters;
    while (w)
    {
        w->done = -1;
        pthread_cond_signal(&w->cond);
        w = w->next;
    }

    pthread_mutex_unlock(&ctx->lock);
    return NULL;
}

/* ================= PUBLIC API ================= */

int nexlink_open(
    const char* serial,
    nexlink_handle_t* out)
{
    nl_ctx_t* ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return -1;

    if (usb_open(serial, &ctx->usb) != 0)
    {
        free(ctx);
        return -1;
    }

    pthread_mutex_init(&ctx->lock, NULL);
    ctx->running = 1;

    pthread_create(
        &ctx->rx_thread,
        NULL,
        rx_thread_fn,
        ctx);

    *out = ctx;
    return 0;
}

void nexlink_close(
    nexlink_handle_t h)
{
    nl_ctx_t *ctx = h;
    if (!ctx)
        return;

    ctx->running = 0;
    pthread_join(ctx->rx_thread, NULL);

    usb_close(ctx->usb);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

int nexlink_cmd(
    nexlink_handle_t h,
    uint8_t cmd,
    const void* payload,
    uint16_t len,
    nexlink_packet_t* resp,
    int timeout_ms)
{
    nl_ctx_t* ctx = h;
    nl_waiter_t waiter = { 0 };
    nexlink_packet_t pkt;
    struct timespec ts;
    int ret = -1;

    pthread_cond_init(&waiter.cond, NULL);

    /* ---------- 注册 waiter ---------- */
    pthread_mutex_lock(&ctx->lock);

    pkt.magic = NL_MAGIC;
    pkt.type = NL_PKT_CMD;
    pkt.cmd = cmd;
    pkt.seq = ++ctx->seq;
    pkt.length = len;

    if (len && payload)
        memcpy(pkt.payload, payload, len);

    waiter.seq = pkt.seq;
    waiter.next = ctx->waiters;
    ctx->waiters = &waiter;

    pthread_mutex_unlock(&ctx->lock);

    /* ---------- 发送（锁外） ---------- */
    if (usb_bulk_write(ctx->usb, &pkt, HEAD_LEN + len) < 0)
        goto out;

    /* ---------- 等待 ---------- */
#if defined(_WIN32)
    int t = timespec_get(&ts, TIME_UTC);
    (void)t;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;

    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&ctx->lock);

    while (!waiter.done)
    {
        int rc = pthread_cond_timedwait(
            &waiter.cond,
            &ctx->lock,
            &ts);

        if (rc == ETIMEDOUT)
            break;
    }

    /* ---------- 从链表移除 ---------- */
    nl_waiter_t** pp = &ctx->waiters;
    while (*pp && *pp != &waiter)
        pp = &(*pp)->next;
    if (*pp)
        *pp = waiter.next;

    if (waiter.done == 1)
    {
        *resp = waiter.resp;
        ret = 0;
    }

    pthread_mutex_unlock(&ctx->lock);

out:
    pthread_cond_destroy(&waiter.cond);
    return ret;
}

int nexlink_send_async(
    nexlink_handle_t h,
    uint8_t cmd,
    const void* payload,
    uint16_t len)
{
    nl_ctx_t* ctx = h;
    nexlink_packet_t pkt;

    pkt.magic = NL_MAGIC;
    pkt.type = NL_PKT_CMD;
    pkt.cmd = cmd;
    pkt.seq = 0;
    pkt.length = len;

    if (len && payload)
        memcpy(pkt.payload, payload, len);

    return usb_bulk_write(
        ctx->usb,
        &pkt,
        HEAD_LEN + len);
}

void nexlink_register_event(
    nexlink_handle_t h,
    nexlink_event_cb cb,
    void* user)
{
    nl_ctx_t* ctx = h;

    pthread_mutex_lock(&ctx->lock);
    ctx->event_cb = cb;
    ctx->event_user = user;
    pthread_mutex_unlock(&ctx->lock);
}
