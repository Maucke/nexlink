#include "usb2uart.h"
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


int UART_Init(unsigned char Channel, PUART_CONFIG pConfig)
{
	return 0;
}

int UART_WriteBytes(unsigned char Channel, unsigned char* pWriteData, int DataSize)
{
	UART_REQUEST request = { .Channel = Channel,.Length = DataSize };
	int ret = control_out(NEX_UART, &request, sizeof request);
	if (ret < 0) return ret;
	ret = data_out(pWriteData, DataSize);
	return ret;
}

int UART_ReadBytes(unsigned char Channel, unsigned char* pReadData, int TimeOutMs)
{
	UART_REQUEST request = { 0 };
	int ret = interrupt_in(&request, sizeof request, TimeOutMs);
	if (ret < 0) return ret;
	ret = data_in(pReadData, request.Length);
	return ret;
}
