#include "nexlink_tx.h"
#include "nexlink_usb_if.h"
#include "usbd_nex_link.h"
#include <string.h>
#include "main.h"

#define TX_SLOT_SIZE  (1280)
#define TX_SLOT_COUNT (4)

/*
 * The host only takes an IN packet when it is actually reading, and
 * USBD_NEX_LINK_TxReady() stays false until it does. So a send must never block
 * waiting for that: frames are queued here and handed to the stack one at a
 * time, keeping the buffer alive until the stack is done with it. A command
 * response is therefore never dropped just because an unsolicited frame
 * (heartbeat) is sitting on the endpoint.
 *
 * nexlink_tx_send() pumps the queue itself: a response must not wait for the
 * main loop, because on boards with a long bring-up (sensors, LCD) the main
 * loop only starts seconds after USB init and the host would time out on its
 * first command. nexlink_tx_poll() is therefore callable from the main loop and
 * from the USB interrupt alike; the queue handoff is atomic so only one context
 * can be sending, and a frame can never be sent twice or overwritten in flight.
 */

static uint8_t tx_slots[TX_SLOT_COUNT][TX_SLOT_SIZE];
static uint16_t tx_lens[TX_SLOT_COUNT];
static volatile uint8_t tx_head;          /* producers: next slot to fill */
static volatile uint8_t tx_tail;          /* consumer: next slot to send */

static uint8_t tx_inflight_buf[TX_SLOT_SIZE];
static volatile bool tx_inflight;         /* stack still owns tx_inflight_buf */

extern USBD_HandleTypeDef hUSB;

#define TX_NEXT(i) ((uint8_t)(((i) + 1U) % TX_SLOT_COUNT))

/* Save/restore PRIMASK: this runs from interrupt as well as thread context, so
   the lock must nest and must not assume interrupts were enabled. */
static inline uint32_t tx_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void tx_unlock(uint32_t primask)
{
    __set_PRIMASK(primask);
}

bool nexlink_tx_send(const void *buf, uint16_t len)
{
    if ((len == 0U) || (len > TX_SLOT_SIZE))
        return false;

    bool queued = false;

    /* The USB interrupt and the main loop both enqueue, so claiming a slot is a
       check-then-write on tx_head and has to be atomic. */
    uint32_t primask = tx_lock();

    uint8_t next = TX_NEXT(tx_head);

    if (next != tx_tail)
    {
        /* Fill the slot at tx_head, then advance tx_head to publish it: the
           consumer reads tx_slots[tx_tail], so filling at TX_NEXT(tx_head)
           would leave the consumer permanently one frame behind. */
        memcpy(tx_slots[tx_head], buf, len);
        tx_lens[tx_head] = len;
        tx_head = next;                   /* publish only once fully written */
        queued = true;
    }

    tx_unlock(primask);

    if (queued)
        nexlink_tx_poll();                /* non-blocking: send now if the bus is free */

    return queued;
}

void nexlink_tx_poll(void)
{
    uint32_t primask = tx_lock();

    if (tx_inflight)
    {
        if (!USBD_NEX_LINK_TxReady(&hUSB))
        {
            tx_unlock(primask);
            return;                       /* buffer still owned by the stack */
        }

        tx_inflight = false;
    }

    if ((tx_head != tx_tail) && USBD_NEX_LINK_TxReady(&hUSB))
    {
        uint8_t idx = tx_tail;
        uint16_t len = tx_lens[idx];

        memcpy(tx_inflight_buf, tx_slots[idx], len);
        tx_tail = TX_NEXT(idx);            /* slot idx is free again */
        tx_inflight = true;

        /* Publish and hand off inside the lock: otherwise another context could
           observe TxReady still true here and send the same slot again. */
        usb_tx(tx_inflight_buf, len);
    }

    tx_unlock(primask);
}
