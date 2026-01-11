#include "myiic.h"
#include "main.h"
#include "usart.h"
#include "mpu6050.h"
#include "stdbool.h"
#include "stdlib.h"
// 初始化MPU6050
// 返回值:0,成功
//     其他,错误代码
u8 MPU_Init(void)
{
  u8 res;
  IIC_Init();								 // 初始化IIC总线
  IIC_Write_Byte(MPU_ADDR, MPU_PWR_MGMT1_REG, 0X80); // 复位MPU6050
  HAL_Delay(100);
  IIC_Write_Byte(MPU_ADDR, MPU_PWR_MGMT1_REG, 0X00); // 唤醒MPU6050
  MPU_Set_Gyro_Fsr(3);					 // 陀螺仪传感器,±2000dps
  MPU_Set_Accel_Fsr(0);					 // 加速度传感器,±2g
  MPU_Set_Rate(50);						 // 设置采样率50Hz//采样率最低为5Hz
  IIC_Write_Byte(MPU_ADDR, MPU_INT_EN_REG, 0X00);	 // 关闭所有中断
  IIC_Write_Byte(MPU_ADDR, MPU_USER_CTRL_REG, 0X00); // I2C主模式关闭
  IIC_Write_Byte(MPU_ADDR, MPU_FIFO_EN_REG, 0X00);	 // 关闭FIFO
  IIC_Write_Byte(MPU_ADDR, MPU_INTBP_CFG_REG, 0X80); // INT引脚低电平有效
  res = IIC_Read_Byte(MPU_ADDR, MPU_DEVICE_ID_REG);
  if(res == MPU_ADDR)  // 器件ID正确
  {
    IIC_Write_Byte(MPU_ADDR, MPU_PWR_MGMT1_REG, 0X01); // 设置CLKSEL,PLL X轴为参考
    IIC_Write_Byte(MPU_ADDR, MPU_PWR_MGMT2_REG, 0X00); // 加速度与陀螺仪都工作
    MPU_Set_Rate(100);						 // 设置采样率为50Hz//采样率最低为5Hz
  }
  else
    return 1;
  return 0;
}

// 设置MPU6050陀螺仪传感器满量程范围
// fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
// 返回值:0,设置成功
//     其他,设置失败
u8 MPU_Set_Gyro_Fsr(u8 fsr)
{
  return IIC_Write_Byte(MPU_ADDR, MPU_GYRO_CFG_REG, fsr << 3); // 设置陀螺仪满量程范围
}

// 设置MPU6050加速度传感器满量程范围
// fsr:0,±2g;1,±4g;2,±8g;3,±16g
// 返回值:0,设置成功
//     其他,设置失败
u8 MPU_Set_Accel_Fsr(u8 fsr)
{
  return IIC_Write_Byte(MPU_ADDR, MPU_ACCEL_CFG_REG, fsr << 3); // 设置加速度传感器满量程范围
}

// 设置MPU6050的数字低通滤波器
// lpf:数字低通滤波频率(Hz)
// 返回值:0,设置成功
//     其他,设置失败
u8 MPU_Set_LPF(u16 lpf)
{
  u8 data = 0;
  if(lpf >= 188)
    data = 1;
  else if(lpf >= 98)
    data = 2;
  else if(lpf >= 42)
    data = 3;
  else if(lpf >= 20)
    data = 4;
  else if(lpf >= 10)
    data = 5;
  else
    data = 6;
  return IIC_Write_Byte(MPU_ADDR, MPU_CFG_REG, data); // 设置数字低通滤波器
}

// 设置MPU6050的采样率(假定Fs=1KHz)
// rate:4~1000(Hz)
// 返回值:0,设置成功
//     其他,设置失败
u8 MPU_Set_Rate(u16 rate)
{
  u8 data;
  if(rate > 1000)
    rate = 1000;
  if(rate < 4)
    rate = 4;
  data = 1000 / rate - 1;
  data = IIC_Write_Byte(MPU_ADDR, MPU_SAMPLE_RATE_REG, data); // 设置数字低通滤波器
  return MPU_Set_LPF(rate / 2);					  // 自动设置LPF为采样率的一半
}

// 得到温度值
// 返回值:温度值(扩大了100倍)
short MPU_Get_Temperature(void)
{
  u8 buf[2];
  short raw;
  float temp;
  IIC_Read_Len(MPU_ADDR, MPU_TEMP_OUTH_REG, 2, buf);
  raw = ((u16)buf[0] << 8) | buf[1];
  temp = 36.53 + ((double)raw) / 340;
  return temp * 100;
  ;
}
// 得到陀螺仪值(原始值)
// gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
// 返回值:0,成功
//     其他,错误代码
u8 MPU_Get_Gyroscope(short* gx, short* gy, short* gz)
{
  u8 buf[6], res;
  res = IIC_Read_Len(MPU_ADDR, MPU_GYRO_XOUTH_REG, 6, buf);
  if(res == 0)
  {
    *gx = ((u16)buf[0] << 8) | buf[1];
    *gy = ((u16)buf[2] << 8) | buf[3];
    *gz = ((u16)buf[4] << 8) | buf[5];
  }
  return res;
}

// 得到加速度值(原始值)
// gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
// 返回值:0,成功
//     其他,错误代码
u8 MPU_Get_Accelerometer(short* ax, short* ay, short* az)
{
  u8 buf[6], res;
  res = IIC_Read_Len(MPU_ADDR, MPU_ACCEL_XOUTH_REG, 6, buf);
  if(res == 0)
  {
    *ax = ((u16)buf[0] << 8) | buf[1];
    *ay = ((u16)buf[2] << 8) | buf[3];
    *az = ((u16)buf[4] << 8) | buf[5];
  }
  return res;
}

bool mpu_debug_en = false;
bool mpu_left, mpu_right, mpu_ok, mpu_quit;
void MPU_CRL(int interval)
{
  static int16_t gx, gy, gz;
  static long  last_update_time;
  long now_tick = HAL_GetTick();
  // dbmsg("HAL_GetTick :%d, last_update_time :%ld", now_tick, last_update_time);
  if(now_tick - last_update_time > interval)
  {
    // dbmsg("HAL_GetTick() - last_update_time :%ld", now_tick- last_update_time);
    MPU_Get_Gyroscope(&gx, &gy, &gz);
    if(gy > 3000)
    {
      mpu_right = true;
      last_update_time = HAL_GetTick() + 400;
      return;
    }
    else if(gy < -3000)
    {
      mpu_left = true;
      last_update_time = HAL_GetTick() + 400;
      return;
    }

    else if(gx > 5000)
    {
      mpu_quit = true;
      last_update_time = HAL_GetTick() + 600;
      return;
    }
    else if(gx < -5000)
    {
      mpu_ok = true;
      last_update_time = HAL_GetTick() + 600;
      return;
    }
    if(gz > 4000)
    {
      mpu_debug_en = true;
      dbmsg("mpu_debug_en: %s", mpu_debug_en ? "true" : "false");
      last_update_time = HAL_GetTick() + 600;
      return;
    }
    if(gz < -4000)
    {
      mpu_debug_en = false;
      dbmsg("mpu_debug_en: %s", mpu_debug_en ? "true" : "false");
      last_update_time = HAL_GetTick() + 600;
      return;
    }

    last_update_time = HAL_GetTick();
  }
}

void MPU_Test(int interval)
{
  static int16_t ax, ay, az;
  static int16_t gx, gy, gz;
  static long  last_update_time;
  long now_tick = HAL_GetTick();
  if(now_tick - last_update_time > interval)
  {
    MPU_Get_Gyroscope(&gx, &gy, &gz);
    dbmsg("G: %d,%d,%d", gx, gy, gz);
    MPU_Get_Accelerometer(&ax, &ay, &az);
    dbmsg("A: %d,%d,%d", ax, ay, az);
    last_update_time = HAL_GetTick();
  }
}
