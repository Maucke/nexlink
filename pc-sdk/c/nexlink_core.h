#pragma once
#include <stdint.h>

#ifdef _WIN32
#define NL_API __declspec(dllexport)
#else
#define NL_API
#endif

#define NL_MAGIC 0xA5

#define HEAD_LEN 8

typedef void* nexlink_handle_t;

typedef enum {
    NL_PKT_CMD   = 0x01,
    NL_PKT_RESP  = 0x02,
    NL_PKT_EVENT = 0x03
} nl_pkt_type_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic;
    uint8_t  type;
    uint16_t  cmd;
    uint16_t seq;
    uint16_t length;
    uint8_t  payload[512];
} nexlink_packet_t;
#pragma pack(pop)

typedef void (*nexlink_event_cb)(
    void *user,
    const nexlink_packet_t *pkt);

/* ===== scan ===== */
NL_API int nexlink_scan(
    char serials[][64],
    int max_count);

/* ===== lifecycle ===== */
NL_API int nexlink_open(
    const char *serial,
    nexlink_handle_t *out);

NL_API void nexlink_close(
    nexlink_handle_t h);

/* ===== command ===== */
NL_API int nexlink_cmd(
    nexlink_handle_t h,
    uint8_t cmd,
    const void *payload,
    uint16_t len,
    nexlink_packet_t *resp,
    int timeout_ms);

/* ===== async command ===== */
NL_API int nexlink_send_async(
    nexlink_handle_t h,
    uint8_t cmd,
    const void *payload,
    uint16_t len);

/* ===== event ===== */
NL_API void nexlink_register_event(
    nexlink_handle_t h,
    nexlink_event_cb cb,
    void *user);
