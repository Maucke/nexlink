#ifndef NEXLINKLIB_H
#define NEXLINKLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

    typedef enum 
    {
        NEX_BREQ_HOST_FORMAT = 0,
        NEX_TIMESTAMP_SET,
        NEX_TIMESTAMP_GET,
        NEX_BRIGHTNESS_SET,
        NEX_BRIGHTNESS_GET,
        NEX_SCREEN_SET,
        NEX_SCREEN_GET,
        NEX_NAME_GET,
        NEX_VERSION_GET,
        NEX_LOG_GET = 0x10,
        NEX_LOG_SIZE_GET,
        NEX_I2C_INIT = 0x20,
        NEX_I2C,
        NEX_UART_INIT = 0x28,
        NEX_UART,
        NEX_COMMAND_LEN,
    }nex_usb_breq;

    typedef struct {
        uint8_t bus_number;
        uint8_t device_address;
        uint16_t vendor_id;
        uint16_t product_id;
        char manufacturer[256];  // 制造商名称
        char product[256];       // 产品名称
        char serial[256];        // 序列号
    } nexlink_device_info_t;

    EXPORT int nexlink_init(void);
    EXPORT int nexlink_scan_devices(nexlink_device_info_t* device_list, int max_devices);
    EXPORT int nexlink_connect_device(uint8_t bus_number, uint8_t device_address);
    EXPORT int nexlink_configure_device(void);
    EXPORT int nexlink_disconnect_device(void);
    EXPORT void nexlink_deinit(void);

    EXPORT int control_in(uint8_t request, void* data, uint16_t size);
    EXPORT int control_out(uint8_t request, const void* data, uint16_t size);
    EXPORT int data_in(void* data, uint16_t size);
    EXPORT int data_out(const void* data, uint16_t size);
    EXPORT int interrupt_in(void* data, uint16_t size, int timeout);

#ifdef __cplusplus
}
#endif

#endif // USB_CAN_LIB_H