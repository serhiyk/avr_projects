#include <avr/io.h>
#include <avr/interrupt.h>
#include "SPI.h"

typedef enum {
    SPI_IDLE = 0,
    SPI_MASTER_TX = 1
} spi_state_t;

typedef struct {
    uint8_t len;
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    uint8_t state;
    spi_cb callback;
} spi_descriptor_t;

static volatile spi_descriptor_t spi_descriptor;

void spi_master_init(void)
{
    /* Set MOSI and SCK output, all others input */
    DDRB |= (1<<PB3)|(1<<PB5)|(1<<PB2);
    /* Enable SPI, Master, set clock rate fck/16 */
    SPCR = (1<<SPIE)|(1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

SIGNAL(SPI_STC_vect)
{
    if (spi_descriptor.rx_buf)
    {
        *spi_descriptor.rx_buf++ = SPDR;
    }
    if (--spi_descriptor.len)
    {
        SPDR = *spi_descriptor.tx_buf++;
    }
    else
    {
        if (spi_descriptor.callback)
        {
            spi_descriptor.callback();
        }
        spi_descriptor.state = SPI_IDLE;
    }
}

uint8_t spi_master_transfer(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t len, spi_cb callback)
{
    if (spi_descriptor.state != SPI_IDLE)
    {
        return -1;
    }
    spi_descriptor.tx_buf = tx_buf + 1;
    spi_descriptor.rx_buf = rx_buf;
    spi_descriptor.len = len;
    spi_descriptor.callback = callback;
    spi_descriptor.state = SPI_MASTER_TX;
    SPDR = *tx_buf;
    return 0;
}

void spi_master_transfer_blocking(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t len)
{
    while (spi_descriptor.state != SPI_IDLE);
    spi_descriptor.tx_buf = tx_buf + 1;
    spi_descriptor.rx_buf = rx_buf;
    spi_descriptor.len = len;
    spi_descriptor.callback = 0;
    spi_descriptor.state = SPI_MASTER_TX;
    SPDR = *tx_buf;
    while (spi_descriptor.state != SPI_IDLE);
}

uint8_t spi_ready(void)
{
    return spi_descriptor.state == SPI_IDLE;
}
