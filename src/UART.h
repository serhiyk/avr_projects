#ifndef UART_H
#define	UART_H

void InitUART(void);
void TransmitByte(uint8_t data);
uint8_t ReceiveByte(void);
void CheckUARTReceiver(void);
void TransmitHex(uint8_t hexNumber);
void SendRAWData(uint8_t data);
void CheckRAWData(void);

#endif	/* UART_H */