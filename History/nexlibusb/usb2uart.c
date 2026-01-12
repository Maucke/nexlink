#include "usb2uart.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#define EXPORT
#include <unistd.h>
#define SLEEP(ms) usleep((ms) * 1000)
#endif


int UART_Init(unsigned char Channel, PUART_CONFIG pConfig)
{
	return 0;
}

int UART_WriteBytes(unsigned char Channel, unsigned char* pWriteData, int DataSize)
{
    int total_size = sizeof(UART_REQUEST) + DataSize;

    unsigned char* buffer = (unsigned char*)malloc(total_size);
    if (buffer == NULL) {
        return -1;
    }

    UART_REQUEST* request = (UART_REQUEST*)buffer;
    request->Cmd = NEX_UART;
    request->Channel = Channel;
    request->Length = (uint8_t)DataSize;

    memcpy(buffer + sizeof(UART_REQUEST), pWriteData, DataSize);

    int ret = data_out(buffer, total_size);
    free(buffer);

    return ret;
}

int UART_ReadBytes(unsigned char Channel, unsigned char* pReadData, int TimeOutMs)
{
    unsigned char* buffer = (unsigned char*)malloc(1024);
	int ret = data_in(buffer, 1024);
    if (ret < 0)return ret;
    UART_REQUEST* request = (UART_REQUEST*)buffer;
    if (request->Cmd != NEX_UART)return -1;
    if (request->Channel != Channel)return -2;

    memcpy(pReadData, buffer + sizeof(UART_REQUEST), request->Length);

	return ret;
}
