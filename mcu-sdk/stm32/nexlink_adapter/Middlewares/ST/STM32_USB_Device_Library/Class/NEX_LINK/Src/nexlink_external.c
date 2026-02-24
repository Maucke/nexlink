#include "nexlink_external.h"
#include "nexlink_app.h"
#include "nexlink_proto.h"
#include <string.h>

#define NL_I2C_MAX 2
#define NL_SPI_MAX 1
#define NL_UART_MAX 2

static nl_i2c_bus_t g_i2c[NL_I2C_MAX] = {
    {
        .hi2c = &hi2c1,
        .clock_hz = 400000,
    },
    {
        .hi2c = &hi2c3,
        .clock_hz = 400000,
    },
};
static nl_spi_bus_t g_spi[NL_SPI_MAX] = {
    {
        .hspi = &hspi1,
        .clock_hz = 1000000,
        .mode = 0,
    },
};
static nl_uart_bus_t g_uart[NL_UART_MAX] = {
    {
        .huart = &huart1,
        .baudrate = 115200,
    },
    {
        .huart = &huart2,
        .baudrate = 115200,
    },
};

static void handle_i2c_config(nl_packet_t *pkt)
{
    if (pkt->length != 5)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint8_t bus = pkt->payload[0];
    uint32_t clk;

    memcpy(&clk, &pkt->payload[1], 4);

    if (bus >= NL_I2C_MAX)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    g_i2c[bus].clock_hz = clk;

    // 如需重新初始化 I2C，可在此重配

    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}

static void handle_i2c_transfer(nl_packet_t *pkt)
{
    if (pkt->length < 7)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint8_t *p = pkt->payload;

    uint8_t bus = *p++;
    uint8_t addr = *p++;
    uint8_t flags = *p++;

    uint16_t wlen;
    uint16_t rlen;

    memcpy(&wlen, p, 2);
    p += 2;
    memcpy(&rlen, p, 2);
    p += 2;

    if (bus >= NL_I2C_MAX)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    I2C_HandleTypeDef *hi2c = g_i2c[bus].hi2c;

    if (wlen > 0)
    {
        HAL_I2C_Master_Transmit(
            hi2c,
            addr << 1,
            p,
            wlen,
            1000);
    }

    uint8_t rbuf[256];

    if (rlen > 0)
    {
        HAL_I2C_Master_Receive(
            hi2c,
            addr << 1,
            rbuf,
            rlen,
            1000);
    }

    send_resp_ok(pkt->cmd, pkt->seq, rbuf, rlen);
}

static void handle_spi_config(nl_packet_t *pkt)
{
    if (pkt->length < 7)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint8_t bus = pkt->payload[0];
    uint32_t clk;

    memcpy(&clk, &pkt->payload[1], 4);

    if (bus >= NL_SPI_MAX)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    g_spi[bus].clock_hz = clk;

    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}
static void handle_spi_transfer(nl_packet_t *pkt)
{
    if (pkt->length < 5)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint8_t *p = pkt->payload;

    uint8_t bus = *p++;
    uint8_t cs = *p++;
    uint8_t flags = *p++;

    uint16_t len;
    memcpy(&len, p, 2);
    p += 2;

    if (bus >= NL_SPI_MAX || len > 512)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    SPI_HandleTypeDef *hspi = g_spi[bus].hspi;

    uint8_t rxbuf[512];

    HAL_SPI_TransmitReceive(
        hspi,
        p,
        rxbuf,
        len,
        1000);

    send_resp_ok(pkt->cmd, pkt->seq, rxbuf, len);
}
static void handle_uart_config(nl_packet_t *pkt)
{
    if (pkt->length < 9)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint8_t bus = pkt->payload[0];

    if (bus >= NL_UART_MAX)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint32_t baud;
    memcpy(&baud, &pkt->payload[1], 4);

    g_uart[bus].baudrate = baud;

    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}
static void handle_uart_write(nl_packet_t *pkt)
{
    if (pkt->length < 3)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    uint8_t *p = pkt->payload;

    uint8_t bus = *p++;

    uint16_t len;
    memcpy(&len, p, 2);
    p += 2;

    if (bus >= NL_UART_MAX)
    {
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_INVALID_PARAM);
        return;
    }

    HAL_UART_Transmit(
        g_uart[bus].huart,
        p,
        len,
        1000);

    send_resp_ok(pkt->cmd, pkt->seq, NULL, 0);
}
void external_handle_cmd(nl_packet_t *pkt)
{
    switch (pkt->cmd)
    {
    case CMD_I2C_CONFIG:
        handle_i2c_config(pkt);
        break;

    case CMD_I2C_TRANSFER:
        handle_i2c_transfer(pkt);
        break;

    case CMD_SPI_CONFIG:
        handle_spi_config(pkt);
        break;

    case CMD_SPI_TRANSFER:
        handle_spi_transfer(pkt);
        break;

    case CMD_UART_CONFIG:
        handle_uart_config(pkt);
        break;

    case CMD_UART_WRITE:
        handle_uart_write(pkt);
        break;
    default:
        send_resp_err(pkt->cmd, pkt->seq, NL_ERR_UNSUPPORTED);
        break;
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (&huart1 == huart)
    {
        uart_data[0].uartId = 0;
        uart_data[0].length = Size;

        send_event(
            EVT_UART_DATA,
            &uart_data[0],
            Size + 3);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_data[0].data, UART_MAX_LEN);
    }
    else if (&huart2 == huart)
    {
        uart_data[1].uartId = 1;
        uart_data[1].length = Size;

        send_event(
            EVT_UART_DATA,
            &uart_data[1],
            Size + 3);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart_data[1].data, UART_MAX_LEN);
    }
}
