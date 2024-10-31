/*

The MIT License (MIT)

Copyright (c) 2016 Hubert Denkmair

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#pragma once

#include <stdint.h>

#define u32 uint32_t
#define u8 uint8_t

#define GSUSB_ENDPOINT_IN1          0x81
#define GSUSB_ENDPOINT_OUT1         0x02
#define GSUSB_ENDPOINT_OUT2         0x03
#define GSUSB_ENDPOINT_OUT3         0x01

typedef enum{
    Tx = 0,  // 发送
    Rx        // 接收
} TxRxMode;

typedef enum{
    PROTOCOL_LOG,   // 提示信息通信
    PROTOCOL_UART = 1,   // 串口通信
    PROTOCOL_I2C,        // I2C通信
    PROTOCOL_SPI,        // SPI通信
    PROTOCOL_CAN,        // CAN通信
    PROTOCOL_ETHERNET,   // 以太网通信
    PROTOCOL_USB,         // USB通信
    PROTOCOL_ERR = 0xff   // 错误
} CommunicationProtocol;

typedef struct {
		unsigned int timestamp;
		unsigned char len;
		CommunicationProtocol type;
		TxRxMode dir;
		unsigned char iserr:1;
		unsigned char iscontinue:1;
		unsigned char reserved:6;
		unsigned char data[64-8];
} LOGData;

enum nex_usb_breq {
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
	NEX_UART_TX,
	NEX_UART_RX,
	NEX_COMMAND_LEN,
};

typedef struct {
	uint16_t size;
	uint16_t maxsize;
	uint16_t isfull;
	uint16_t reserve;
}nex_log_des;

typedef struct {
	uint16_t brightness;
	uint16_t damp;
}nex_brightness_des;

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t blocksize;
	uint8_t direction:2;
	uint8_t reserved:6;
}nex_screen_des;

typedef struct {
	uint64_t timestamp_s;
	nex_brightness_des brides;
	nex_screen_des scrdes;
}nex_usb_des;

typedef struct {  
	uint8_t channel;
	uint8_t reserved1;
	uint16_t reserved2;         
	uint32_t baudRate;        
} nex_i2c_init;

#define MAX_DATA_SIZE (64-8)  // 定义最大数据缓冲区大小
typedef struct {
	uint8_t channel;
	uint8_t deviceAddress;      // I2C 从设备地址
	uint8_t dataWriteLength;         // Write数据长度
	uint8_t dataReadLength;         // Read数据长度
	uint16_t cycle;        		    // cycle时间（毫秒）
	uint16_t timeout;            // 超时时间（毫秒）
	uint8_t dataWriteBuffer[MAX_DATA_SIZE];    
//	uint8_t dataReadBuffer[MAX_DATA_SIZE];         
} nex_i2c_request;

typedef struct {
	uint8_t channel;
	TxRxMode dir;
	uint16_t dataWriteLength;         // 数据长度
	uint32_t reserved2;
	uint8_t dataWriteBuffer[MAX_DATA_SIZE];    
//	uint8_t dataReadBuffer[MAX_DATA_SIZE];         
} nex_uart_request;
