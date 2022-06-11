#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "config.h"

// Tested on 1 MHz

uint8_t onewire_master_reset(void)
{
    uint8_t result = 1;
    cli();
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);

    if (!(ONEWIRE_PORT_IN & (1 << ONEWIRE_PIN)))
    {
        // busy
        goto exit;
    }

    // reset
    ONEWIRE_DDR |= (1 << ONEWIRE_PIN);
    _delay_us(480);
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);

    uint8_t present = 0;
    for (uint8_t i = 48; i > 0; i--)
    {
        if (!(ONEWIRE_PORT_IN & (1 << ONEWIRE_PIN)))
        {
            present = 1;
        }
        _delay_us(10);
    }
    if (!present)
    {
        goto exit;
    }

    _delay_us(5);
    result = 0;
exit:
    sei();
    return result;
}

void onewire_master_write(uint8_t data)
{
    cli();
    for (uint8_t i = 8; i > 0; i--)
    {
        ONEWIRE_DDR |= (1 << ONEWIRE_PIN);
        _delay_us(1);
        if (data & 0x01)
        {
            ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
        }
        _delay_us(55);
        ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
        _delay_us(5);
        data >>= 1;
    }
    sei();
}

uint8_t onewire_master_read(void)
{
    uint8_t data = 0;

    cli();
    for (uint8_t i = 8; i > 0; i--)
    {
        ONEWIRE_DDR |= (1 << ONEWIRE_PIN);
        _delay_us(1);
        ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
        _delay_us(4);
        data >>= 1;
        if (ONEWIRE_PORT_IN & (1 << ONEWIRE_PIN))
        {
            data |= 0x80;
        }
        _delay_us(60);
     }

    _delay_us(5);
    sei();

    return data;
}
