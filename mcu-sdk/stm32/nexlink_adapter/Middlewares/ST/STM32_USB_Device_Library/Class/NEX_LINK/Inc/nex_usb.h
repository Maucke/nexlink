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

#define NEXUSB_BULK_ENDPOINT_IN1 0x81
#define NEXUSB_BULK_ENDPOINT_OUT1 0x02
#define NEXUSB_INT_ENDPOINT_IN 0x83

typedef enum
{
	Tx = 0,
	Rx
} TxRxMode;

typedef enum
{
	PROTOCOL_LOG,
	PROTOCOL_UART = 1,
	PROTOCOL_I2C,
	PROTOCOL_SPI,
	PROTOCOL_CAN,
	PROTOCOL_ETHERNET,
	PROTOCOL_USB,
	PROTOCOL_ERR = 0xff
} CommunicationProtocol;

typedef struct
{
	unsigned int timestamp;
	unsigned char len;
	CommunicationProtocol type;
	TxRxMode dir;
	unsigned char iserr : 1;
	unsigned char iscontinue : 1;
	unsigned char reserved : 6;
	unsigned char data[64 - 8];
} LOGData;

enum nex_usb_breq
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
};

typedef struct
{
	uint16_t size;
	uint16_t maxsize;
	uint16_t isfull;
	uint16_t reserve;
} nex_log_des;

typedef struct
{
	uint16_t brightness;
	uint16_t damp;
} nex_brightness_des;

typedef struct
{
	uint16_t width;
	uint16_t height;
	uint16_t blocksize;
	uint8_t direction : 2;
	uint8_t reserved : 6;
} nex_screen_des;

typedef struct
{
	uint64_t timestamp_s;
	nex_brightness_des brides;
	nex_screen_des scrdes;
} nex_usb_des;

typedef struct
{
	uint8_t channel;
	uint8_t reserved1;
	uint16_t reserved2;
	uint32_t baudRate;
} nex_i2c_init;

typedef struct
{
	uint8_t channel;
	uint8_t deviceAddress; // 8bit addr
	uint8_t length;
	uint8_t reserved;
	uint16_t timeout;
} nex_i2c_request;

typedef struct
{
	uint8_t reserved;
	uint8_t channel;
	uint16_t length;
	uint32_t reserved2;
} nex_uart_request;
