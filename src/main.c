#include "matrix.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "DS3231.h"
#include "UART.h"
#include <stdint.h>

#define BOOTLOADER_START_ADDRESS 0x3800

SIGNAL(INT1_vect)
{
    /*if(PIND & (1 << PD3))
        uart_send_byte('1');
    else
        uart_send_byte('0');*/
    //ADCSRA |= 1 << ADSC;
}

void adc_init(void)
{
    // AREF = AVcc
    ADMUX = (1 << REFS0);

    DIDR0 = 1 << ADC0D;

    ADCSRB |= (1 << ADTS1);

    // ADC Enable and prescaler of 128
    // 16000000/128 = 125000
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) | (1 << ADATE) | (0 << ADIE);
}

static volatile uint8_t adc_f=1;
static volatile uint16_t l;

/*SIGNAL(ADC_vect)
{
    l = ADC;
}*/

void bootloader_handler(uint8_t byte)
{
    static uint8_t index = 0;
    static const uint8_t reset_command[] = {0x00, 0x18, 0xF8};

    if (byte == reset_command[index]) {
        index++;
        if (index == sizeof(reset_command)) {
            ((void (*)(void))BOOTLOADER_START_ADDRESS)();
        }
    }
}

void UART_handler(void)
{
    uint8_t tmp;

    do {
        tmp = uart_get_byte();
        bootloader_handler(tmp);
    } while (uart_check_receiver());
}

void setup(void)
{

    ds3231_init();
    uart_init();
    asm("sei");
    //_delay_ms(5);
    Matrix_Init();

    //PORTD |= 1 << PD3;
    EICRA |= 1 << ISC10;    // Any logical change on INT1 generates an interrupt request
    EIMSK |= 1 << INT1; // External Interrupt Request 1 Enable

    adc_init();

    asm("sei");
}

int main(void)
{
    setup();

  //set_time(0,1,18,1,22,6,14);
    while(1)
    {
        ds3231_handler(TimeUpdateHandler);
        MatrixUpdateHandler();
        //_delay_ms(1000);
        //uart_send_byte('a');
        //PORTB ^= 1 << PB5;
        if (uart_check_receiver()) {
            UART_handler();
        }
    }
}
