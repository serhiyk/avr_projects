#ifndef NRF24L01_H
#define NRF24L01_H

typedef void (*nrf_cb)(uint8_t *buf);

void nrf24l01_init(void);
uint8_t nrf24_register_cb(uint8_t pipe, uint8_t *addr, uint8_t addr_len, uint8_t rx_len, nrf_cb callback);
void nrf24_set_tx_addr(uint8_t *addr, uint8_t addr_len);
void nrf24_send(uint8_t *tx_buf, uint8_t len);
void nrf24_handler(void);

#endif  /* NRF24L01_H */
