#include "display.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "DS3231.h"
#include "UART.h"
#include "timer.h"
#include <stdint.h>
#include "motion.h"
#include "serial.h"
#include "nrf24l01.h"
#include "sensors.h"

// #define MOTION_SENSOR_ENABLED
#define RTC_ENABLED
#define DISPLAY_ENABLED

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

void setup(void)
{
    spi_master_init();
    serial_init();
#ifdef RTC_ENABLED
    ds3231_init();
#endif
    timer_init();
    asm("sei");
    //_delay_ms(5);
#ifdef DISPLAY_ENABLED
    display_init();
#endif

    //PORTD |= 1 << PD3;
    // EICRA |= 1 << ISC10;    // Any logical change on INT1 generates an interrupt request
    // EIMSK |= 1 << INT1; // External Interrupt Request 1 Enable

    adc_init();

    asm("sei");

#ifdef MOTION_SENSOR_ENABLED
    motion_init();
#else
    display_activate();
#endif
    nrf24l01_init();
    sensors_init();
}

void t_handler(void)
{
    uart_send_byte((get_date_bcd()>>4)+0x30);
    uart_send_byte((get_date_bcd()&0x0F)+0x30);
    uart_send_byte('-');
    uart_send_byte((get_month()&0x0F)+0x30);
    uart_send_byte(' ');
    uart_send_byte((get_hour_bcd()>>4)+0x30);
    uart_send_byte((get_hour_bcd()&0x0F)+0x30);
    uart_send_byte(':');
    uart_send_byte((get_minute_bcd()>>4)+0x30);
    uart_send_byte((get_minute_bcd()&0x0F)+0x30);
    uart_send_byte(':');
    uart_send_byte((get_second_bcd()>>4)+0x30);
    uart_send_byte((get_second_bcd()&0x0F)+0x30);
    uart_send_byte('\n');
    uart_send_byte('\r');
}

int main(void)
{
    setup();
    uart_send_byte('S');
    // set_time(0,11,21,3,22,3,16);
    while(1)
    {
#ifdef RTC_ENABLED
        ds3231_handler(time_update_handler);
#endif
#ifdef DISPLAY_ENABLED
        display_handler();
#endif
        serial_handler();
        nrf24_handler();
        timer_handler();
    }
}
