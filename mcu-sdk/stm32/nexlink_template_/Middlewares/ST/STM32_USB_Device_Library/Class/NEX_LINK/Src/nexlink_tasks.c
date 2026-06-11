#include "nexlink_tasks.h"
#include "nexlink_ringbuf.h"
#include "nexlink_app.h"
#include "nexlink_tx.h"

void NexLinkRxProcess(void)
{
    while (ringbuf_available())
    {
        uint8_t buf[1024];
        uint16_t n = ringbuf_read(buf, sizeof(buf));
        nexlink_rx_bytes(buf, n);
    }
}

void NexLinkInit(void)
{
    ringbuf_init();
    nexlink_tx_init();
}
