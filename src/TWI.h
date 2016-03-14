#ifndef TWI_H
#define TWI_H

#include <stdint.h>

void twi_init(void);
typedef void (*twi_cb)(void);
uint8_t twi_master_transfer(uint8_t dev_addr, uint8_t *tx_buf, uint8_t *rx_buf, uint8_t tx_len, uint8_t rx_len, twi_cb callback);
uint8_t twi_ready(void);

#define twi_master_send(dev_addr,tx_buf,tx_len,callback) twi_master_transfer(dev_addr,tx_buf,0,tx_len,0,callback)
#define twi_master_receive(dev_addr,rx_buf,rx_len,callback) twi_master_transfer(dev_addr,0,rx_buf,0,rx_len,callback)

#endif /* TWI_H */
