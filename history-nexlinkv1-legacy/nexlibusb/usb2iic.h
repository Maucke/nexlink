#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

	//定义函数返回错误代码  
#define IIC_SUCCESS                          (0)   //函数执行成功  
#define IIC_ERR_NOT_SUPPORT       (-1)  //适配器不支持该函数  
#define IIC_ERR_USB_WRITE_FAIL  (-2)  //USB写数据失败  
#define IIC_ERR_USB_READ_FAIL    (-3)  //USB读数据失败  
#define IIC_ERR_CMD_FAIL               (-4)  //命令执行失败  
#define IIC_ERR_PARA_ERROR          (-5)  //参数传入错误  
//定义IIC函数返回错误代码  
#define IIC_ERROR_SUCCESS           0   //操作成功  
#define IIC_ERROR_CHANNEL           1   //该通道不支持该函数  
#define IIC_ERROR_BUSY                 2   //总线忙  
#define IIC_ERROR_START_FAILD   3   //启动总线失败  
#define IIC_ERROR_TIMEOUT          4   //超时  
#define IIC_ERROR_NACK                5   //从机无应答  
#define IIC_ERROR_PEC                   6   //PEC校验失败  

//定义初始化IIC的数据类型  
	typedef struct _IIC_CONFIG {
		unsigned int    ClockSpeedHz;   //IIC时钟频率:单位为Hz  
		unsigned char   AddrBits;         //从机地址模式，7-7bit模式，10-10bit模式  
		unsigned char   Reserve;         //预留
	}IIC_CONFIG, * PIIC_CONFIG;

	typedef struct
	{
		uint8_t Channel;
		uint8_t DeviceAddress; // 8bit addr
		uint8_t Length;
		uint8_t Reserved;
		uint16_t Timeout;
	} IIC_REQUEST;

	EXPORT int IIC_Init(int IICIndex, PIIC_CONFIG pConfig);
	EXPORT int IIC_GetSlaveAddr(int IICIndex, short* pSlaveAddr);
	EXPORT int IIC_WriteBytes(int IICIndex, short SlaveAddr, unsigned char* pWriteData, int WriteLen);
	EXPORT int IIC_ReadBytes(int IICIndex, short SlaveAddr, unsigned char* pReadData, int ReadLen);

#ifdef __cplusplus
}
#endif
