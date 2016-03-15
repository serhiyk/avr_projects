#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_send_byte(uint8_t data);
uint8_t uart_get_byte(void);
uint8_t uart_check_receiver(void);
void uart_send_hex(uint8_t hexNumber);

#endif  /* UART_H */
