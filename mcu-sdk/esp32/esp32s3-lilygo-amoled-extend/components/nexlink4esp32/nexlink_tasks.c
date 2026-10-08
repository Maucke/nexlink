#include "nexlink_tasks.h"
#include "nexlink_tx.h"
#include "nexlink_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "esp_mac.h"
#include "tusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "esp_log.h"
#include "esp_system.h"

TaskHandle_t nexlink_task_handle;

extern const tusb_desc_device_t desc_device;
extern uint8_t const desc_configuration[];
extern const char *string_desc[];

/*
 * NexLinkTask 平时阻塞在 ulTaskNotifyTake() 上，靠 USB 回调唤醒；
 * 这个超时只是兜底（事件丢了也不至于卡太久）。
 *
 * 注意本工程 CONFIG_FREERTOS_HZ = 100，pdMS_TO_TICKS(1) == 0，
 * vTaskDelay(0) 是不阻塞的 —— 用它当轮询间隔会让这个优先级 5 的任务
 * 永远就绪、把 IDLE0 饿死，5 秒后触发 task_wdt。所以下面必须保证
 * 超时至少是一个 tick。
 */
#define NEXLINK_IDLE_TIMEOUT ((pdMS_TO_TICKS(50) != 0) ? pdMS_TO_TICKS(50) : 1)

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

/*
 * 收到数据 / 一次 IN 传输完成（vendor FIFO 腾出空间）：唤醒 NexLinkTask。
 *
 * 这两个回调由 tud_task() 调用（vendord_xfer_cb），是任务上下文，可以直接
 * xTaskNotifyGive。这里刻意不解析报文、也不往 FIFO 里写 —— 那等于在 tud_task
 * 里重入 USB 栈，交给 NexLinkTask 做。
 */
void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    (void)itf;
    (void)buffer;
    (void)bufsize;

    if (nexlink_task_handle)
        xTaskNotifyGive(nexlink_task_handle);
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes)
{
    (void)itf;
    (void)sent_bytes;

    if (nexlink_task_handle)
        xTaskNotifyGive(nexlink_task_handle);
}

void NexLinkTask(void *arg)
{
    uint8_t buf[1024];

    for (;;)
    {
        /* RX：把 FIFO 里现有的都读完，且不受 TX 状态影响 ——
           主机不读时 TX 会停在半路，但命令必须继续收 */
        while (tud_vendor_available())
        {
            uint32_t n = tud_vendor_read(buf, sizeof(buf));
            if (n)
                nexlink_rx_bytes(buf, n);
        }

        /* TX：能写多少写多少（FIFO 满就返回），剩下的等 tx 回调再唤醒 */
        nexlink_tx_poll();

        ulTaskNotifyTake(pdTRUE, NEXLINK_IDLE_TIMEOUT);
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
        /* ESP32-S3 只有 Full Speed（TUD_OPT_HIGH_SPEED == 0），用普通配置描述符 */
        .configuration_descriptor = desc_configuration,
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
