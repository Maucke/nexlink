#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libusb.h"
#include "nexlibusb.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#define EXPORT
#include <unistd.h>
#define SLEEP(ms) usleep((ms) * 1000)
#endif

// USB constants
static const int USB_DIR_OUT = 0;
static const int USB_DIR_IN = 0x80;
static const int USB_TYPE_VENDOR = (0x02 << 5);
static const int USB_RECIP_INTERFACE = 0x01;


#pragma pack(push, 1)
#pragma pack(pop)

// Global variables
static libusb_context* usb_context = NULL;
static libusb_device_handle* device_handle = NULL;
static int is_initialized = 0;

EXPORT int control_in(uint8_t request, void* data, uint16_t size) {
    if (!device_handle) {
        return -1;
    }
    return libusb_control_transfer(device_handle,
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
        request, 0, 0,
        (unsigned char*)data, size, 1000);
}

EXPORT int control_out(uint8_t request, const void* data, uint16_t size) {
    if (!device_handle) {
        return -1;
    }
    return libusb_control_transfer(device_handle,
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
        request, 0, 0,
        (unsigned char*)data, size, 1000);
}

EXPORT int data_in(void* data, uint16_t size) {
    int transferred;
    if (!device_handle) {
        return -1;
    }
    int ret = libusb_bulk_transfer(device_handle, 0x81,
        (unsigned char*)data, size, &transferred, 1000);
    return ret >= 0 ? transferred : ret;
}

EXPORT int data_out(const void* data, uint16_t size) {
    int transferred;
    if (!device_handle) {
        return -1;
    }
    int ret = libusb_bulk_transfer(device_handle, 0x02,
        (unsigned char*)data, size, &transferred, 1000);
    return ret >= 0 ? transferred : ret;
}

EXPORT int nexlink_init(void) {
    if (is_initialized) {
        return 0; // Already initialized
    }

    int ret = libusb_init(&usb_context);
    if (ret < 0) {
        return ret;
    }

    is_initialized = 1;
    return 0;
}

EXPORT void nexlink_cleanup(void) {
    if (device_handle) {
        libusb_close(device_handle);
        device_handle = NULL;
    }

    if (usb_context) {
        libusb_exit(usb_context);
        usb_context = NULL;
    }

    is_initialized = 0;
}

EXPORT int nexlink_scan_devices(nexlink_device_info_t *device_list, int max_devices) {
    if (!is_initialized) {
        return -1;
    }
    
    if (!device_list || max_devices <= 0) {
        return -2;  // 参数错误
    }
    
    libusb_device** devs;
    ssize_t cnt = libusb_get_device_list(usb_context, &devs);
    if (cnt < 0) {
        return (int)cnt;
    }
    
    int found_count = 0;
    
    for (int i = 0; devs[i] && found_count < max_devices; ++i) {
        struct libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(devs[i], &desc);
        if (ret < 0) continue;
        
        // Check for gs_usb device (OpenMoko vendor)
        if (desc.idVendor == 0x1d50 && desc.idProduct == 0x606f) {
            nexlink_device_info_t *dev_info = &device_list[found_count];
            
            // 获取基本信息
            dev_info->bus_number = libusb_get_bus_number(devs[i]);
            dev_info->device_address = libusb_get_device_address(devs[i]);
            dev_info->vendor_id = desc.idVendor;
            dev_info->product_id = desc.idProduct;
            
            // 临时打开设备以获取字符串描述符
            libusb_device_handle *temp_handle;
            ret = libusb_open(devs[i], &temp_handle);
            if (ret == LIBUSB_SUCCESS) {
                // 获取制造商名称
                if (desc.iManufacturer > 0) {
                    ret = libusb_get_string_descriptor_ascii(temp_handle, 
                                                           desc.iManufacturer,
                                                           (unsigned char*)dev_info->manufacturer,
                                                           sizeof(dev_info->manufacturer));
                    if (ret < 0) {
                        strcpy_s(dev_info->manufacturer, 256, "Unknown Manufacturer");
                    }
                } else {
                    strcpy_s(dev_info->manufacturer, 256, "No Manufacturer");
                }
                
                // 获取产品名称
                if (desc.iProduct > 0) {
                    ret = libusb_get_string_descriptor_ascii(temp_handle,
                                                           desc.iProduct,
                                                           (unsigned char*)dev_info->product,
                                                           sizeof(dev_info->product));
                    if (ret < 0) {
                        strcpy_s(dev_info->product, 256, "Unknown Product");
                    }
                } else {
                    strcpy_s(dev_info->product, 256, "No Product Name");
                }
                
                // 获取序列号
                if (desc.iSerialNumber > 0) {
                    ret = libusb_get_string_descriptor_ascii(temp_handle,
                                                           desc.iSerialNumber,
                                                           (unsigned char*)dev_info->serial,
                                                           sizeof(dev_info->serial));
                    if (ret < 0) {
                        strcpy_s(dev_info->serial, 256, "Unknown Serial");
                    }
                } else {
                    strcpy_s(dev_info->serial, 256, "No Serial");
                }
                
                libusb_close(temp_handle);
            }
            
            found_count++;
        }
    }
    
    libusb_free_device_list(devs, 1);
    return found_count;
}

EXPORT int nexlink_connect_device(uint8_t bus_number, uint8_t device_address) {
    if (!is_initialized) {
        return -1;
    }
    
    // 如果已经有设备连接，先关闭
    if (device_handle) {
        libusb_close(device_handle);
        device_handle = NULL;
    }
    
    libusb_device** devs;
    ssize_t cnt = libusb_get_device_list(usb_context, &devs);
    if (cnt < 0) {
        return (int)cnt;
    }
    
    int result = -1;  // 默认未找到设备
    
    for (int i = 0; devs[i]; ++i) {
        // 检查总线号和设备地址是否匹配
        if (libusb_get_bus_number(devs[i]) == bus_number &&
            libusb_get_device_address(devs[i]) == device_address) {
            
            struct libusb_device_descriptor desc;
            int ret = libusb_get_device_descriptor(devs[i], &desc);
            if (ret < 0) continue;
            
            // 再次确认这是我们要的设备类型
            if (desc.idVendor == 0x1d50 && desc.idProduct == 0x606f) {
                ret = libusb_open(devs[i], &device_handle);
                if (ret == LIBUSB_SUCCESS) {
                    result = 0;  // 成功
                    break;
                } else {
                    result = ret;  // 返回libusb错误码
                    break;
                }
            } else {
                result = -2;  // 设备类型不匹配
                break;
            }
        }
    }
    
    libusb_free_device_list(devs, 1);
    return result;
}

EXPORT int nexlink_open_device(void) {
    nexlink_device_info_t devices[1];
    int found = nexlink_scan_devices(devices, 1);
    
    if (found <= 0) {
        return found;  // 没找到设备或出错
    }
    
    return nexlink_connect_device(devices[0].bus_number, devices[0].device_address);
}

EXPORT void nexlink_print_device_info(const nexlink_device_info_t *dev_info) {
    if (!dev_info) return;
    
    printf("Device Information:\n");
    printf("  Bus: %d, Address: %d\n", dev_info->bus_number, dev_info->device_address);
    printf("  VID:PID = %04x:%04x\n", dev_info->vendor_id, dev_info->product_id);
    printf("  Manufacturer: %s\n", dev_info->manufacturer);
    printf("  Product: %s\n", dev_info->product);
    printf("  Serial: %s\n", dev_info->serial);
    printf("\n");
}

EXPORT void nexlink_get_device_display_name(const nexlink_device_info_t *dev_info, char *display_name, size_t max_len) {
    if (!dev_info || !display_name || max_len == 0) return;
    
    if (strlen(dev_info->manufacturer) > 0 && strcmp(dev_info->manufacturer, "Unknown Manufacturer") != 0) {
        if (strlen(dev_info->product) > 0 && strcmp(dev_info->product, "Unknown Product") != 0) {
            snprintf(display_name, max_len, "%s - %s", dev_info->manufacturer, dev_info->product);
        } else {
            snprintf(display_name, max_len, "%s", dev_info->manufacturer);
        }
    } else if (strlen(dev_info->product) > 0 && strcmp(dev_info->product, "Unknown Product") != 0) {
        snprintf(display_name, max_len, "%s", dev_info->product);
    } else {
        snprintf(display_name, max_len, "Device %04x:%04x", dev_info->vendor_id, dev_info->product_id);
    }
}

EXPORT int nexlink_configure_device(void) {
    if (!device_handle) {
        return -1;
    }

    // Get and set configuration
    struct libusb_config_descriptor* config;
    int ret = libusb_get_config_descriptor(libusb_get_device(device_handle), 0, &config);
    if (ret < 0) return ret;

    ret = libusb_set_configuration(device_handle, config->bConfigurationValue);
    libusb_free_config_descriptor(config);
    if (ret < 0) return ret;

    // Claim interface
    ret = libusb_claim_interface(device_handle, 0);
    if (ret < 0) return ret;

    ret = libusb_set_interface_alt_setting(device_handle, 0, 0);
    if (ret < 0) return ret;

    return 0;
}
