#ifndef SPI_H
#define SPI_H

#include <stdint.h>

typedef void (*spi_cb)(void);
void spi_master_init(void);
uint8_t spi_master_transfer(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t len, spi_cb callback);
void spi_master_transfer_blocking(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t len);
uint8_t spi_ready(void);

#define spi_master_send(tx_buf,len,callback) spi_master_transfer(tx_buf,0,len,callback)
#define spi_master_send_blocking(tx_buf,len) spi_master_transfer_blocking(tx_buf,0,len)

#endif /* SPI_H */
