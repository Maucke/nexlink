#pragma once
#include "nexlink_core.h"
#include "pthread.h"

typedef struct nl_waiter {
    uint16_t seq;
    int done;                   // 0 waiting / 1 done / -1 abort
    nexlink_packet_t resp;
    pthread_cond_t cond;
    struct nl_waiter *next;
} nl_waiter_t;

typedef struct {
    void *usb;

    pthread_t rx_thread;
    pthread_mutex_t lock;

    uint8_t rx_buf[2048];
    uint16_t rx_len;

    uint16_t seq;
    nl_waiter_t *waiters;

    nexlink_event_cb event_cb;
    void *event_user;

    int running;
} nl_ctx_t;
