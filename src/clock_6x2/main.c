#include "display.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "DS3231.h"
#include "bmp180.h"
#include "UART.h"
#include "SPI.h"
#include "TWI.h"
#include "timer.h"
#include <stdint.h>
#include "serial.h"
#include "nrf24l01.h"
#include "max7219.h"
#include "sensors.h"

#define MOTION_SENSOR_ENABLED
#define RTC_ENABLED
#define DISPLAY_ENABLED

#define MOTION_TIMER_ID 1
#define MOTION_TIMER_PERIOD 10 // 1 second
#define MOTION_PIN (PIND & (1 << PD6))
#define IDLE_COUNTER_INIT_VALUE 2

#define LED_CONTROL_PORT PORTB
#define LED_CONTROL_DDR DDRB
#define LED_CONTROL_PIN PB1

#define ILLUMINANCE_ON_VALUE 150
#define ILLUMINANCE_OFF_VALUE 250

static uint8_t idle_counter=0;

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

SIGNAL(PCINT2_vect)
{
    // motion interrupt
}

inline static void motion_handler(void)
{
    if (idle_counter == 0 && MOTION_PIN)
    {
        if (ADC < ILLUMINANCE_ON_VALUE)
        {
            LED_CONTROL_PORT |= 1 << LED_CONTROL_PIN;
        }
        display_activate();
        idle_counter = IDLE_COUNTER_INIT_VALUE;
    }
}

void motion_timer(void)
{
    if (MOTION_PIN)
    {
        if (ADC < ILLUMINANCE_ON_VALUE)
        {
            LED_CONTROL_PORT |= 1 << LED_CONTROL_PIN;
        }
        else if (ADC > ILLUMINANCE_OFF_VALUE)
        {
            LED_CONTROL_PORT &= ~(1 << LED_CONTROL_PIN);
        }
        if (idle_counter != 0xFF && idle_counter != 0)
        {
            idle_counter++;
        }
    }
    else if (idle_counter)
    {
        idle_counter--;
        if (idle_counter == 0)
        {
            LED_CONTROL_PORT &= ~(1 << LED_CONTROL_PIN);
            display_deactivate();
        }
    }
}

void setup(void)
{
    LED_CONTROL_DDR |= 1 << LED_CONTROL_PIN;
    LED_CONTROL_PORT &= ~(1 << LED_CONTROL_PIN);
    twi_init();
    spi_master_init();
    serial_init();
    timer_init();
    asm("sei");
#ifdef RTC_ENABLED
    ds3231_init();
#endif
    bmp180_init(0, print_pressure);
    //_delay_ms(5);
#ifdef DISPLAY_ENABLED
    display_init();
#endif

    adc_init();

    asm("sei");

#ifdef MOTION_SENSOR_ENABLED
    PCMSK2 |= 1 << PCINT22;
    PCICR |= 1 << PCIE2;  // Pin Change Interrupt Enable 2
    timer_register(MOTION_TIMER_ID, MOTION_TIMER_PERIOD, motion_timer);
#else
    display_activate();
#endif
    nrf24l01_init();
    sensors_init();
}

int main(void)
{
    setup();
    uart_send_byte('S');
    // set_time(0,11,21,3,22,3,16);
    while(1)
    {
#ifdef MOTION_SENSOR_ENABLED
        motion_handler();
#endif
#ifdef RTC_ENABLED
        ds3231_handler(time_update_handler);
#endif
#ifdef DISPLAY_ENABLED
        max7219_handler();
#endif
        serial_handler();
        nrf24_handler();
        bmp180_handler();
        timer_handler();
    }
}
