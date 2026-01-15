#include "nexlink_tasks.h"
#include "nexlink_tx.h"
#include "nexlink_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "esp_mac.h"
#include "tinyusb.h"
#include "tusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "esp_log.h"
#include "esp_system.h"

TaskHandle_t nexlink_task_handle;
extern QueueHandle_t txq;

extern const tusb_desc_device_t desc_device;
extern const uint8_t desc_configuration[];
extern const char *string_desc[];
extern uint8_t const desc_configuration[];

static char serial_str[13]; // 12 hex + '\0'
void init_usb_serial_from_mac(void)
{
    uint64_t mac;
    esp_efuse_mac_get_default((uint8_t *)&mac);

    snprintf(serial_str, sizeof(serial_str),
             "%02X%02X%02X%02X%02X%02X",
             (uint8_t)(mac >> 40),
             (uint8_t)(mac >> 32),
             (uint8_t)(mac >> 24),
             (uint8_t)(mac >> 16),
             (uint8_t)(mac >> 8),
             (uint8_t)(mac));
}

void NexLinkTask(void *arg)
{
    uint8_t buf[1024];
    tx_item_t tx;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (tud_vendor_available())
        {
            uint32_t n = tud_vendor_read(buf, sizeof(buf));
            if (n)
                nexlink_rx_bytes(buf, n);
        }
        if (!tud_vendor_write_available())
            continue;
        if (xQueuePeek(txq, &tx, 0) != pdTRUE)
            continue;
        usb_tx(tx.buf, tx.len);
        xQueueReceive(txq, &tx, 0);
    }
}

void NexLinkInit(void)
{
    init_usb_serial_from_mac();
    string_desc[3] = serial_str;
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &desc_device,
        .string_descriptor = string_desc,
        .external_phy = false,
#if TUD_OPT_HIGH_SPEED
        .fs_configuration_descriptor = desc_configuration,
        .hs_configuration_descriptor = NULL,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = desc_configuration,
#endif
    };
    tinyusb_config_cdcacm_t acm_cfg = {0};
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    esp_tusb_init_console(TINYUSB_CDC_ACM_0);

    nexlink_tx_init();
    xTaskCreate(
        NexLinkTask,
        "NexLinkTask",
        1024 * 4, NULL, 5, &nexlink_task_handle);
}
