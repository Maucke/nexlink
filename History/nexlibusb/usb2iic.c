#include "usb2iic.h"
#include "nexlibusb.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#define EXPORT
#include <unistd.h>
#include "usb2iic.h"
#define SLEEP(ms) usleep((ms) * 1000)
#endif

int IIC_Init(int IICIndex, PIIC_CONFIG pConfig)
{
	return 0;
}

int IIC_GetSlaveAddr(int IICIndex, short* pSlaveAddr)
{
	return 0;
}

int IIC_WriteBytes(int IICIndex, short SlaveAddr, unsigned char* pWriteData, int WriteLen)
{
	IIC_REQUEST request = { .Channel = IICIndex, .DeviceAddress = SlaveAddr << 1, .Length = WriteLen };
	int ret = control_out(NEX_I2C, &request, sizeof request);
	if (ret < 0) return ret;
	ret = data_out(pWriteData, WriteLen);
	return ret;
}

int IIC_ReadBytes(int IICIndex, short SlaveAddr, unsigned char* pReadData, int ReadLen)
{
	IIC_REQUEST request = { .Channel = IICIndex, .DeviceAddress = (SlaveAddr << 1) | 1, .Length = ReadLen };
	int ret = control_out(NEX_I2C, &request, sizeof request);
	if (ret < 0) return ret;
	ret = data_in(pReadData, ReadLen);
	return ret;
}
