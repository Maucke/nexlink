#pragma once

#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint32_t clock_hz;
} nl_i2c_bus_t;

typedef struct
{
    SPI_HandleTypeDef *hspi;
    uint32_t clock_hz;
    uint8_t mode;
} nl_spi_bus_t;

typedef struct
{
    UART_HandleTypeDef *huart;
    uint32_t baudrate;
} nl_uart_bus_t;

typedef enum
{
    PIN_FREE = 0,
    PIN_I2C,
    PIN_SPI,
    PIN_UART,
    PIN_GPIO
} pin_owner_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    pin_owner_t owner;
} nl_pin_t;

#ifdef __cplusplus
}
#endif
