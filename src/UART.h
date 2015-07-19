#ifndef UART_H
#define	UART_H

void InitUART(void);
void TransmitByte(uint8_t data);
uint8_t ReceiveByte(void);
uint8_t CheckUARTReceiver(void);
void TransmitHex(uint8_t hexNumber);

#endif	/* UART_H */