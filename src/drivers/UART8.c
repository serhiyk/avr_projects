#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include "UART.h"
#include "config.h"

#define UART_RX_BUFFER_SIZE 256 //UART Receive Buffer Size /* 2,4,8,16,32,64,128 or 256 bytes */
#define UART_TX_BUFFER_SIZE 256 //UART Transmit Buffer Size

#ifndef BAUDRATE
#define BAUDRATE 57600
#endif

static volatile uint8_t UART_RxBuf[UART_RX_BUFFER_SIZE];    //UART Receive Buffer
static volatile uint8_t UART_RxHead;
static volatile uint8_t UART_RxTail;
static volatile uint8_t UART_TxBuf[UART_TX_BUFFER_SIZE];    //UART Transmit Buffer
static volatile uint8_t UART_TxHead;
static volatile uint8_t UART_TxTail;

static volatile uint8_t UART_RxFlag=0;

void uart_init(void)
{
    PORTD |= 0x02;

    UBRRH = (uint8_t) (((F_CPU / (8UL * BAUDRATE)) - 1) >> 8);  // set baud rate
    UBRRL = (uint8_t) ((F_CPU / (8UL * BAUDRATE)) - 1);
    UCSRA |= (1 << U2X);
    UCSRB = (1 << RXCIE) | (1 << RXEN) | (1 << TXEN);

    UART_RxTail = 0;    //Set UART Buffer
    UART_RxHead = 0;
    UART_TxTail = 0;
    UART_TxHead = 0;
}

void uart_update_baudrate(uint32_t baudrate)
{
    UBRRH = (uint8_t) (((F_CPU / (8UL * baudrate)) - 1) >> 8);  // set baud rate
    UBRRL = (uint8_t) ((F_CPU / (8UL * baudrate)) - 1);
}

uint8_t uart_check_receiver(void)
{
    return UART_RxFlag;
}

void uart_send_byte(uint8_t data) //UART Transmit Byte
{
    uint8_t tmphead;

    tmphead = UART_TxHead + 1;
    while(tmphead == UART_TxTail);
    UART_TxBuf[tmphead] = data;
    UART_TxHead = tmphead;
    UCSRB |= (1 << UDRIE);    //Enable Interrupt UART Data Register Empty
}

uint8_t uart_get_byte(void)   //UART Receive Byte
{
    uint8_t tmptail;

    while(UART_RxHead == UART_RxTail);
    tmptail = UART_RxTail + 1;
    UART_RxTail = tmptail;
    if(UART_RxHead == UART_RxTail)
        UART_RxFlag = 0;

    return UART_RxBuf[tmptail];
}

SIGNAL(USART_RXC_vect)   //Interrupt UART Receive Byte
{
    uint8_t tmphead;

    tmphead = UART_RxHead + 1;
    UART_RxBuf[tmphead] = UDR;
    UART_RxHead = tmphead;
    UART_RxFlag = 1;
}

SIGNAL(USART_UDRE_vect) //Interrupt UART Data Register Empty (Transmit Byte)
{
    uint8_t tmptail;


    if(UART_TxHead != UART_TxTail)  //If Transmit Buffer Not Free, Transmit Byte
    {
        tmptail = UART_TxTail + 1;
        UART_TxTail = tmptail;
        UDR = UART_TxBuf[tmptail];
    }
    else    //If Transmit Buffer Free
    {
        UCSRB &= ~(1 << UDRIE);   //Disable Interrupt UART Data Register Empty
    }
}

/********************************************************************************
Conversion HEX to ASCII and send to UART
********************************************************************************/
void uart_send_hex(uint8_t hexNumber)
{
    uint8_t tmp;

    tmp = hexNumber >> 4;
    if(tmp > 9) tmp = (tmp - 9) | 0x40;
    else tmp |= 0x30;
    uart_send_byte(tmp);
    hexNumber &= 0x0F;
    if(hexNumber > 9) hexNumber = (hexNumber - 9) | 0x40;
    else hexNumber |= 0x30;
    uart_send_byte(hexNumber);
}
