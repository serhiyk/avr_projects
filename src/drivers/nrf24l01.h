#ifndef NRF24L01_H
#define NRF24L01_H

void nrf24l01_init(void);
void nrf24_send(uint8_t* tx_buf, uint8_t len);
void nrf24_handler(void);

#endif  /* NRF24L01_H */
