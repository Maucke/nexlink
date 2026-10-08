#include "nexlink_app.h"
#include "nexlink_proto.h"
#include "nexlink_tx.h"
#include "tinyusb.h"
#include "tusb.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
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
static volatile bool g_host_seen = false; /* 收到过命令 = 上位机连上过 */

/* 图像接收（CMD_FRAME_*）状态，实现在 handle_cmd 之前 */
static void frame_reset(void);

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

bool nexlink_host_connected(void)
{
    return g_host_seen;
}

/* 掉线（设备被复位/去配置）后不再主动推事件。
   注意：上位机只是关闭句柄、没有复位设备时这个回调不会触发，
   那种情况靠 nexlink_tx.c 里"发送卡住 → nexlink_event_disable()"兜底。 */
void tud_umount_cb(void)
{
    g_event_enabled = false;
    g_host_seen = false;
    frame_reset(); /* 丢掉可能收了一半的帧，别把残留画到下一位主机上 */
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

/* ================= 图像接收：上位机 → 设备屏幕 =================
 *
 * 上位机的顺序（见 pc-sdk NexLinkDevice.SendFrame / SendFrameFast）：
 *   CMD_FRAME_BEGIN  payload = width(2) height(2) bpp(1)   → 要应答
 *   CMD_FRAME_DATA   payload = 顺序像素字节，1000 字节一包   → 不回包
 *   CMD_FRAME_END    无 payload                             → 要应答
 *
 * 这里只管协议侧：累计长度、攒够若干整行后一次性交给 nexlink_display_flush()。
 * 数据包边界和行边界不对齐（1000 vs 1072），所以必须自己攒；攒多行再画是为了少
 * 发几次 SPI 传输 —— 每次 draw 都会阻塞调用者（RX 任务），画得太碎会拖慢收包。
 *
 * 每收完一帧往 CDC 串口打一行统计（总耗时 / 画图耗时 / 等效速率 / FPS），
 * 用来区分"慢在 USB 还是慢在屏"。
 */

#define FRAME_LINE_MAX_PIXELS 1024 /* 支持的屏宽上限（16bpp） */
#define FRAME_ROWS_PER_FLUSH 8     /* 攒几行画一次；越大 SPI 传输次数越少 */

static uint16_t frame_w;
static uint16_t frame_h;
static uint32_t frame_expected; /* 整帧字节数 */
static uint32_t frame_received;
static int frame_row_bytes;  /* 一行字节数 */
static int frame_row_y;      /* 下一行要收的行号 */
static int frame_stage_used; /* 暂存缓冲已填字节数 */
static int frame_stage_rows; /* 暂存缓冲能攒几行 */
static bool frame_active;
static bool frame_bad; /* 长度对不上等异常，整帧作废 */

/* 暂存缓冲 ping-pong 两块（DMA 可访问），跨帧复用不释放：
   交给面板去传的那一块，等下一批行攒的时候不会再被写 —— 不用去赌底层驱动
   "到底什么时候传完"。两块够用的依据：面板驱动每次 draw_bitmap 都会先把在飞的
   传输收完（IDF 的 esp_lcd_panel_io_spi.c 里 tx_param/tx_color 开头就 recycle
   在飞事务），所以同一时刻最多只有一块在飞。 */
static uint8_t *frame_stage[2];
static int frame_stage_idx; /* 正在攒的那一块 */
static size_t frame_stage_size;

/* 统计：组件只负责累计，打印交给应用侧（nexlink_frame_stat()），
   免得在收帧路径里做可能阻塞的串口输出 */
static int64_t frame_t_begin_us;
static int64_t frame_draw_us;
static uint32_t frame_draw_calls;
static nexlink_frame_stat_t frame_stat;

static void frame_reset(void)
{
    frame_active = false;
    frame_bad = false;
    frame_received = 0;
    frame_row_y = 0;
    frame_stage_used = 0;
    frame_stage_idx = 0;
}

/* 把暂存里攒满的整行交给屏幕，然后换另一块继续攒 */
static void frame_flush(int rows)
{
    if (rows <= 0)
        return;

    int64_t t0 = esp_timer_get_time();
    nexlink_display_flush(0, frame_row_y, frame_w, frame_row_y + rows, frame_stage[frame_stage_idx]);
    frame_draw_us += esp_timer_get_time() - t0;
    frame_draw_calls++;

    frame_stage_idx ^= 1; /* 刚交出去那块留给驱动读，换另一块 */
    frame_row_y += rows;
    frame_stage_used = 0;
}

static void handle_frame_begin(nl_packet_t *pkt)
{
    if (pkt->length < 5)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint16_t w, h;
    uint8_t bpp;
    memcpy(&w, pkt->payload + 0, 2);
    memcpy(&h, pkt->payload + 2, 2);
    bpp = pkt->payload[4];

    frame_reset();

    /* 目前只支持 RGB565（上位机视频流用的就是它） */
    if ((bpp != NL_BPP_RGB565) || (w == 0) || (h == 0) || (w > FRAME_LINE_MAX_PIXELS))
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    frame_w = w;
    frame_h = h;
    frame_row_bytes = (int)w * 2;
    frame_expected = (uint32_t)frame_row_bytes * h;

    /* 暂存缓冲：两块 ping-pong，尽量各攒 FRAME_ROWS_PER_FLUSH 行，内存不够就往下退 */
    if (frame_stage_size < (size_t)frame_row_bytes * FRAME_ROWS_PER_FLUSH)
    {
        for (int rows = FRAME_ROWS_PER_FLUSH; rows >= 1; rows >>= 1)
        {
            size_t need = (size_t)frame_row_bytes * rows;

            frame_stage[0] = heap_caps_malloc(need, MALLOC_CAP_DMA);
            frame_stage[1] = frame_stage[0] ? heap_caps_malloc(need, MALLOC_CAP_DMA) : NULL;

            if (frame_stage[0] && frame_stage[1])
            {
                frame_stage_size = need;
                break;
            }

            /* 两块没凑齐就把这次拿到的都放掉，再退一档重试 */
            for (int i = 0; i < 2; i++)
            {
                if (frame_stage[i])
                {
                    heap_caps_free(frame_stage[i]);
                    frame_stage[i] = NULL;
                }
            }
        }

        if ((frame_stage[0] == NULL) || (frame_stage[1] == NULL))
        {
            frame_stage_size = 0;
            send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INTERNAL);
            return;
        }
    }
    frame_stage_rows = (int)(frame_stage_size / (size_t)frame_row_bytes);

    frame_t_begin_us = esp_timer_get_time();
    frame_draw_us = 0;
    frame_draw_calls = 0;
    frame_active = true;
    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

/* 数据包是 SendAsync 发的，上位机不等应答，所以这里不回包 */
static void handle_frame_data(nl_packet_t *pkt)
{
    if (!frame_active || (frame_stage[0] == NULL))
        return;

    if (frame_received + pkt->length > frame_expected)
    {
        frame_bad = true; /* 多了：整帧作废，END 时回错 */
        return;
    }

    const uint8_t *src = pkt->payload;
    uint16_t left = pkt->length;

    while (left > 0)
    {
        int space = (int)frame_stage_size - frame_stage_used;
        int n = ((int)left < space) ? (int)left : space;

        memcpy(frame_stage[frame_stage_idx] + frame_stage_used, src, n);
        frame_stage_used += n;
        src += n;
        left -= (uint16_t)n;
        frame_received += (uint32_t)n;

        if (frame_stage_used == (int)frame_stage_size)
            frame_flush(frame_stage_rows);
    }
}

static void handle_frame_end(nl_packet_t *pkt)
{
    if (!frame_active || frame_bad || (frame_received != frame_expected))
    {
        frame_reset();
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    /* 收尾：最后没攒满的整行也画出去（不足一行补 0） */
    if (frame_stage_used > 0)
    {
        int rows = (frame_stage_used + frame_row_bytes - 1) / frame_row_bytes;
        memset(frame_stage[frame_stage_idx] + frame_stage_used, 0,
               (size_t)rows * frame_row_bytes - frame_stage_used);
        frame_flush(rows);
    }

    /* 累计统计，由应用侧（log_task 之类）周期性取走打串口 */
    int64_t now = esp_timer_get_time();
    frame_stat.frames++;
    frame_stat.bytes += frame_expected;
    frame_stat.draw_us += (uint32_t)frame_draw_us;
    frame_stat.draw_calls += frame_draw_calls;
    frame_stat.last_total_ms = (uint32_t)((now - frame_t_begin_us) / 1000);
    frame_stat.last_draw_ms = (uint32_t)(frame_draw_us / 1000);
    frame_stat.last_w = frame_w;
    frame_stat.last_h = frame_h;

    frame_reset();
    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

/* 取走并清零统计数据（应用侧周期调用，比如 1Hz 打到 CDC 串口） */
void nexlink_frame_stat(nexlink_frame_stat_t *out)
{
    if (out == NULL)
        return;

    *out = frame_stat;
    memset(&frame_stat, 0, sizeof(frame_stat));
}

/* 应用侧可覆盖：把一行 RGB565 画到屏幕。默认什么都不做（工程不接屏也能编过） */
__attribute__((weak)) void nexlink_display_flush(int x1, int y1, int x2, int y2, const uint8_t *rgb565)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)rgb565;
}

/* 应用侧可覆盖：报告屏幕信息（上位机发图前要问）。默认不支持 */
__attribute__((weak)) bool nexlink_display_info(nl_display_t *info)
{
    (void)info;
    return false;
}

static void handle_get_display_info(nl_packet_t *pkt)
{
    nl_display_t info;
    uint8_t payload[1 + sizeof(nl_display_t)];

    if (!nexlink_display_info(&info))
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_UNSUPPORTED);
        return;
    }

    payload[0] = 1; /* display_count */
    memcpy(payload + 1, &info, sizeof(nl_display_t));
    send_resp_ok(pkt->cmd, pkt->seq, payload, sizeof(payload));
}

static void handle_cmd(nl_packet_t *pkt)
{
    g_event_enabled = true;
    g_host_seen = true; /* 上位机连上就会先发 CMD_GET_VERSION */
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

    case CMD_FRAME_BEGIN:
        handle_frame_begin(pkt);
        break;

    case CMD_FRAME_DATA:
        handle_frame_data(pkt); /* 纯数据流，不回包 */
        break;

    case CMD_FRAME_END:
        handle_frame_end(pkt);
        break;

    case CMD_FRAME_GET:
        /* 设备 → 上位机传屏需要一份整帧缓存（536x240x2 ≈ 257KB），
           本工程没开 PSRAM，暂不支持 */
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_UNSUPPORTED);
        break;

    case CMD_GET_DISPLAY_INFO:
        handle_get_display_info(pkt);
        break;

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
