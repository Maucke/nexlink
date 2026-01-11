#include "myiic.h"

void delay_us(uint16_t us)
{
	uint16_t i,j;
	for(i=0;i<us;i++)
		for(j=0;j<10;j++)
	;
//	HAL_Delay(1);
}
// 初始化IIC
void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOB_CLK_ENABLE();   //使能GPIOB时钟
    
    //PH4,5初始化设置
    GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_9;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FAST;     //快速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

	IIC_SCL = 1;
	IIC_SDA = 1;
}
// 产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT(); // sda线输出
	IIC_SDA = 1;
	IIC_SCL = 1;
	delay_us(4);
	IIC_SDA = 0; // START:when CLK is high,DATA change form high to low
	delay_us(4);
	IIC_SCL = 0; // 钳住I2C总线，准备发送或接收数据
}
// 产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT(); // sda线输出
	IIC_SCL = 0;
	IIC_SDA = 0; // STOP:when CLK is high DATA change form low to high
	delay_us(4);
	IIC_SCL = 1;
	IIC_SDA = 1; // 发送I2C总线结束信号
	delay_us(4);
}
// 等待应答信号到来
// 返回值：1，接收应答失败
//         0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	SDA_IN(); // SDA设置为输入
	IIC_SDA = 1;
	delay_us(1);
	IIC_SCL = 1;
	delay_us(1);
	while (READ_SDA)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL = 0; // 时钟输出0
	return 0;
}
// 产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 0;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(2);
	IIC_SCL = 0;
}
// 不产生ACK应答
void IIC_NAck(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(2);
	IIC_SCL = 0;
}
// IIC发送一个字节
// 返回从机有无应答
// 1，有应答
// 0，无应答
void IIC_Send_Byte(u8 txd)
{
	u8 t;
	SDA_OUT();
	IIC_SCL = 0; // 拉低时钟开始数据传输
	for (t = 0; t < 8; t++)
	{
		IIC_SDA = (txd & 0x80) >> 7;
		txd <<= 1;
		delay_us(2); // 对TEA5767这三个延时都是必须的
		IIC_SCL = 1;
		delay_us(2);
		IIC_SCL = 0;
		delay_us(2);
	}
}
// 读1个字节，ack=1时，发送ACK，ack=0，发送nACK
u8 IIC_Recv_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	SDA_IN(); // SDA设置为输入
	for (i = 0; i < 8; i++)
	{
		IIC_SCL = 0;
		delay_us(2);
		IIC_SCL = 1;
		receive <<= 1;
		if (READ_SDA)
			receive++;
		delay_us(1);
	}
	if (!ack)
		IIC_NAck(); // 发送nACK
	else
		IIC_Ack(); // 发送ACK
	return receive;
}

// IIC写一个字节
// reg:寄存器地址
// data:数据
// 返回值:0,正常
//     其他,错误代码
u8 IIC_Write_Byte(u8 addr, u8 reg, u8 data)
{
	IIC_Start();
	IIC_Send_Byte((addr << 1) | 0); // 发送器件地址+写命令
	if (IIC_Wait_Ack())					// 等待应答
	{
		IIC_Stop();
		return 1;
	}
	IIC_Send_Byte(reg);	 // 写寄存器地址
	IIC_Wait_Ack();		 // 等待应答
	IIC_Send_Byte(data); // 发送数据
	if (IIC_Wait_Ack())	 // 等待ACK
	{
		IIC_Stop();
		return 1;
	}
	IIC_Stop();
	return 0;
}

// IIC读一个字节
// reg:寄存器地址
// 返回值:读到的数据
u8 IIC_Read_Byte(u8 addr, u8 reg)
{
	u8 res;
	IIC_Start();
	IIC_Send_Byte((addr << 1) | 0); // 发送器件地址+写命令
	IIC_Wait_Ack();						// 等待应答
	IIC_Send_Byte(reg);					// 写寄存器地址
	IIC_Wait_Ack();						// 等待应答
	IIC_Start();
	IIC_Send_Byte((addr << 1) | 1); // 发送器件地址+读命令
	IIC_Wait_Ack();						// 等待应答
	res = IIC_Recv_Byte(0);				// 读取数据,发送nACK
	IIC_Stop();							// 产生一个停止条件
	return res;
}

// IIC连续写
// addr:器件地址
// reg:寄存器地址
// len:写入长度
// buf:数据区
// 返回值:0,正常
//     其他,错误代码
u8 IIC_Write_Len(u8 addr, u8 reg, u8 len, u8 *buf)
{
	u8 i;
	IIC_Start();
	IIC_Send_Byte((addr << 1) | 0); // 发送器件地址+写命令
	if (IIC_Wait_Ack())				// 等待应答
	{
		IIC_Stop();
		return 1;
	}
	IIC_Send_Byte(reg); // 写寄存器地址
	IIC_Wait_Ack();		// 等待应答
	for (i = 0; i < len; i++)
	{
		IIC_Send_Byte(buf[i]); // 发送数据
		if (IIC_Wait_Ack())	   // 等待ACK
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_Stop();
	return 0;
}

// IIC连续读
// addr:器件地址
// reg:要读取的寄存器地址
// len:要读取的长度
// buf:读取到的数据存储区
// 返回值:0,正常
//     其他,错误代码
u8 IIC_Read_Len(u8 addr, u8 reg, u8 len, u8 *buf)
{
	IIC_Start();
	IIC_Send_Byte((addr << 1) | 0); // 发送器件地址+写命令
	if (IIC_Wait_Ack())				// 等待应答
	{
		IIC_Stop();
		return 1;
	}
	IIC_Send_Byte(reg); // 写寄存器地址
	IIC_Wait_Ack();		// 等待应答
	IIC_Start();
	IIC_Send_Byte((addr << 1) | 1); // 发送器件地址+读命令
	IIC_Wait_Ack();					// 等待应答
	while (len)
	{
		if (len == 1)
			*buf = IIC_Recv_Byte(0); // 读数据,发送nACK
		else
			*buf = IIC_Recv_Byte(1); // 读数据,发送ACK
		len--;
		buf++;
	}
	IIC_Stop(); // 产生一个停止条件
	return 0;
}
