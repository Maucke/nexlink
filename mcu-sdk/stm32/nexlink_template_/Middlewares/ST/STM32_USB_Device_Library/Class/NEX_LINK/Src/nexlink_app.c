#include "nexlink_app.h"
#include "nexlink_proto.h"
#include "nexlink_tx.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "main.h"
#include "nexlink_rampool.h"

#define NEXLINK_LOG_MAX_LEN 96
#define LCD_WIDTH 30
#define LCD_HEIGHT 20

static const nl_display_info_t g_display_info =
    {
        .display_count = 1,
        .displays =
            {
                {
                    .width = LCD_WIDTH,
                    .height = LCD_HEIGHT,
                    .bpp = Rgb565,
                    .refresh = 30,
                }}};


static uint8_t rx_buf[NL_MAX_PAYLOAD];
static uint16_t rx_len;

static uint64_t mcu_time_ms(void)
{
    return HAL_GetTick();
}

static void send_resp_internal(
    uint16_t cmd,
    uint16_t seq,
    uint8_t status,
    const void *payload,
    uint16_t len)
{
    /* RESP payload = status(1) + data */
    if (len > NL_MAX_PAYLOAD - 1)
        return;

    uint16_t payload_len = 1 + len;
    uint16_t frame_len = HEAD_LEN + payload_len;

    if (frame_len > BUF_SIZE)
        return;
		
    uint8_t *tx = buf_alloc(frame_len);
    if (!tx)
        return;

    nl_packet_t *pkt = (nl_packet_t *)tx;
    pkt->magic = NL_MAGIC;
    pkt->type = NL_PKT_RESP;
    pkt->cmd = cmd;
    pkt->seq = seq;
    pkt->length = payload_len;

    uint8_t *p = pkt->payload;
    p[0] = status;

    if (len && payload)
        memcpy(p + 1, payload, len);

    nexlink_tx_send(tx, frame_len);
}

void send_resp_err(
    uint16_t cmd,
    uint16_t seq,
    nl_err_t err)
{
    send_resp_internal(cmd, seq, (uint8_t)err, NULL, 0);
}

void send_resp_ok(
    uint16_t cmd,
    uint16_t seq,
    const void *payload,
    uint16_t len)
{
    send_resp_internal(cmd, seq, NL_ERR_OK, payload, len);
}

void send_event(
    uint16_t cmd,
    const void *payload,
    uint16_t len)
{
    if (len > NL_MAX_PAYLOAD)
        return;

    uint16_t frame_len = HEAD_LEN + len;

    if (frame_len > BUF_SIZE)
        return;
		
    uint8_t *tx = buf_alloc(frame_len);
    if (!tx)
        return;

    nl_packet_t *pkt = (nl_packet_t *)tx;
    pkt->magic = NL_MAGIC;
    pkt->type = NL_PKT_EVENT;
    pkt->cmd = cmd;
    pkt->seq = 0;
    pkt->length = len;

    if (len && payload)
        memcpy(pkt->payload, payload, len);

    nexlink_tx_auto(tx, frame_len);
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

void nex_send_heartbeat(void)
{
    uint8_t payload[9];

    uint32_t uptime = HAL_GetTick();
    uint16_t vcc = 1;   
    uint8_t state = 1;  
    uint8_t err = 0;

    memcpy(&payload[0], &uptime, 4);
    payload[4] = state;
    payload[5] = err;
    memcpy(&payload[6], &vcc, 2);
    payload[8] = 0;

    send_event(EVT_HEARTBEAT, payload, sizeof(payload));
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

static uint16_t frame_width = 0;
static uint16_t frame_height = 0;
static uint8_t frame_bpp = 0;

static uint32_t frame_expected_size = 0;
static uint32_t frame_received = 0;

#define MAX_FRAME_SIZE 1024
static uint8_t frame_buffer[MAX_FRAME_SIZE];

static void handle_frame_start(nl_packet_t *pkt)
{
    if (pkt->length < 5)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }
    memcpy(&frame_width, pkt->payload + 0, 2);
    memcpy(&frame_height, pkt->payload + 2, 2);
    frame_bpp = pkt->payload[4];

    if (frame_bpp != Dual2Color)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    frame_expected_size = ((frame_width + 3) / 4) * frame_height;
    frame_received = 0;
    // nexlink_log("frame_expected_size: %d, frame_width: %d, frame_height: %d",frame_expected_size,frame_width,frame_height);

    if (frame_expected_size > MAX_FRAME_SIZE)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_NO_MEM);
        return;
    }

    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

static void handle_frame_data(nl_packet_t *pkt)
{
    // nexlink_log("frame_expected_size: %d, frame_received: %d, pkt->length: %d",frame_expected_size,frame_received,pkt->length);
    if (frame_received + pkt->length > frame_expected_size)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    memcpy(frame_buffer + frame_received,
           pkt->payload,
           pkt->length);

    frame_received += pkt->length;

    //    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

int pack_frame_buffer(uint8_t *frame_buffer,
                      uint16_t frame_width,
                      uint16_t frame_height)
{
    uint32_t index = 0;

    for (uint16_t y = 0; y < frame_height; y++)
    {
        for (uint16_t x = 0; x < frame_width; x += 4)
        {
            uint8_t out_byte = 0;

            for (int i = 0; i < 4; i++)
            {
                uint16_t px = x + i;
                uint8_t color = 0;

                if (px < frame_width)
                {
                    // uint8_t red = mn11236_read_pixel(px, y, MN11236_COLOR_RED);
                    // uint8_t blue = mn11236_read_pixel(px, y, MN11236_COLOR_BLUE);

                    // if (red)
                    //     color |= 0x01;
                    // if (blue)
                    //     color |= 0x02;
                }

                out_byte |= (color & 0x03) << (6 - i * 2);
            }

            frame_buffer[index++] = out_byte;
        }
    }
    return index;
}

static void handle_frame_end(nl_packet_t *pkt)
{
    if (frame_received != frame_expected_size)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }
    // mn11236_clear_buffers();
    uint32_t index = 0;

    for (uint16_t y = 0; y < frame_height; y++)
    {
        for (uint16_t x = 0; x < frame_width; x += 4)
        {
            uint8_t byte = frame_buffer[index++];

            for (int i = 0; i < 4; i++)
            {

                uint16_t px = x + i;
                if (px < frame_width)
                {
										// uint8_t color = (byte >> (6 - i * 2)) & 0x03;
                    // if (color)
                    // {
                    //     if (color & 1)
                    //         mn11236_draw_pixel(px, y, MN11236_COLOR_RED);
                    //     if (color & 2)
                    //         mn11236_draw_pixel(px, y, MN11236_COLOR_BLUE);
                    // }
                    // else
                    // {
                    //     mn11236_clear_pixel(px, y, MN11236_COLOR_RED);
                    //     mn11236_clear_pixel(px, y, MN11236_COLOR_BLUE);
                    // }
                }
            }
        }
    }
    // mn11236_send_buffer();
    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

static uint8_t uploadframeflag = 0;

static void handle_frame_get(nl_packet_t *pkt)
{
    uploadframeflag = pkt->payload[0];

    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

static uint32_t upload_frame_expected_size = 0;
static uint8_t upload_frame_buffer[MAX_FRAME_SIZE];
void upload_frame_upload()
{
    uint32_t upload_offset = 0;
    uint32_t chunk_size = 1000;
    uint8_t startPayload[5];

    uint16_t width = LCD_WIDTH;
    uint16_t height = LCD_HEIGHT;
    uint8_t bpp = Dual2Color;

    if (uploadframeflag == 0)
        return;
    if (uploadframeflag < 0xFF)
        uploadframeflag--;

    /* Little Endian */
    startPayload[0] = (uint8_t)(width & 0xFF);
    startPayload[1] = (uint8_t)((width >> 8) & 0xFF);

    startPayload[2] = (uint8_t)(height & 0xFF);
    startPayload[3] = (uint8_t)((height >> 8) & 0xFF);

    startPayload[4] = bpp;

    send_event(EVT_FRAME_UPLOAD_BEGIN, startPayload,
               sizeof startPayload);

    upload_frame_expected_size = pack_frame_buffer(upload_frame_buffer, width, height);
    while (upload_offset < upload_frame_expected_size)
    {
        uint32_t size = upload_frame_expected_size - upload_offset;
        if (size > chunk_size)
            size = chunk_size;
        //				nexlink_log("frame_buffer: %d, size: %d, offset: %d",frame_buffer,size, offset);
        send_event(EVT_FRAME_UPLOAD_DATA, upload_frame_buffer + upload_offset,
                   size);

        upload_offset += size;
    }

    send_event(EVT_FRAME_UPLOAD_END, NULL,
               0);
}

__weak void external_handle_cmd(nl_packet_t *pkt)
{
    switch (pkt->cmd)
    {
        default:
            send_resp_err(pkt->cmd, pkt->seq, NL_ERR_UNSUPPORTED);
            break;
    }
}

static void handle_cmd(nl_packet_t *pkt)
{
    switch (pkt->cmd)
    {
    case CMD_HW_RESET:
        send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
        HAL_NVIC_SystemReset();
        break;
    case CMD_PING:
        send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
        break;

    case CMD_LOOPBACK:
        nexlink_cmd_loopback(pkt);
        break;

    case CMD_SYNC_TIME:
    {
        uint64_t pc_time = 0;

        if (pkt->length == sizeof(uint64_t))
            memcpy(&pc_time, pkt->payload, sizeof(uint64_t));

        uint64_t mcu_time = mcu_time_ms();

//        struct tm *tm_local;
//        long timestamp_s;
//        timestamp_s = pc_time / 1000;
//        timestamp_s += 3600 * 8;
//        tm_local = localtime((const time_t *)&timestamp_s);
//        RX8900_SetTime(tm_local);
        send_resp_ok(pkt->cmd, pkt->seq, &mcu_time, sizeof(mcu_time));
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

    case CMD_KEY:
        switch (pkt->payload[0])
        {
        case UpArrow:
            break;
        case DownArrow:
            break;
        case RightArrow:
            break;
        case LeftArrow:
            break;
        default:
            send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
            return;
        }
        send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
        break;

    case CMD_GET_DISPLAY_INFO:
        send_resp_ok(pkt->cmd, pkt->seq, &g_display_info, sizeof g_display_info);
        break;

    case CMD_FRAME_BEGIN:
        handle_frame_start(pkt);
        break;

    case CMD_FRAME_DATA:
        handle_frame_data(pkt);
        break;

    case CMD_FRAME_END:
        handle_frame_end(pkt);
        break;

    case CMD_FRAME_GET:
        handle_frame_get(pkt);
        break;

    default:
        external_handle_cmd(pkt);
        break;
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
        if (rx_len < total)
            return;

        nl_packet_t *pkt = (nl_packet_t *)rx_buf;
        if (pkt->type == NL_PKT_CMD)
            handle_cmd(pkt);

        memmove(rx_buf, rx_buf + total, rx_len - total);
        rx_len -= total;
    }
}
