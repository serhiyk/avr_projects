#include <avr/io.h>
#include <avr/interrupt.h>
#include "UART.h"
#include "TWI.h"
#include "DS3231.h"

#define DS3231_ADDRESS 0xD0

static uint8_t TWIbuf[19];

volatile uint8_t RTC_int_flag;

void ClearAlarm(void)
{
    static uint8_t buf[] = {0x00};
    while (TWI_master_send(DS3231_ADDRESS, 15, buf, 1));
}

uint8_t getSecond(void)
{
    return bcdToDec(TWIbuf[0]);
}

uint8_t getSecondBCD(void)
{
    return TWIbuf[0];
}

uint8_t getMinute(void)
{
    return bcdToDec(TWIbuf[1]);
}

uint8_t getMinuteBCD(void)
{
    return TWIbuf[1];
}

uint8_t getHour(void)
{
    uint8_t temp_buffer;
    uint8_t hour;
    temp_buffer = TWIbuf[2];
    if(temp_buffer & 0b01000000)
        hour = bcdToDec(temp_buffer & 0b00011111);
    else
        hour = bcdToDec(temp_buffer & 0b00111111);
    return hour;
}

uint8_t getHourBCD(void)
{
    return TWIbuf[2];
}

uint8_t getDoW(void)
{
    return bcdToDec(TWIbuf[3]);
}

uint8_t getDate(void)
{
    return bcdToDec(TWIbuf[4]);
}

uint8_t getDateBCD(void)
{
    return TWIbuf[4];
}

uint8_t getMonth(void)
{
    return (bcdToDec(TWIbuf[5] & 0b01111111)) ;
}

uint8_t getYear(void)
{
    return bcdToDec(TWIbuf[6]);
}

void setTime(uint8_t Second, uint8_t Minute, uint8_t Hour, uint8_t DoW, uint8_t Date, uint8_t Month, uint8_t Year)
{
    // while(TWIstate);
    // TWIstate = TWI_MASTER_TX;

    // TWIbuf[0] = decToBcd(Second);
    // TWIbuf[1] = decToBcd(Minute);
    // TWIbuf[2] = decToBcd(Hour);// & 0b10111111;
    // TWIbuf[3] = decToBcd(DoW);
    // TWIbuf[4] = decToBcd(Date);
    // TWIbuf[5] = decToBcd(Month);
    // TWIbuf[6] = decToBcd(Year);
    // TWIbuf[7] = 0x80;
    // TWIbuf[8] = 0x80;
    // TWIbuf[9] = 0x80;
    // TWIbuf[10] = 0x80;
    // TWIbuf[11] = 0x80;
    // TWIbuf[12] = 0x80;
    // TWIbuf[13] = 0x80;
    // TWIbuf[14] = 0x05; // Alarm 1 Interrupt Enable
    // TWIbuf[15] = 0x00;

    // TWIaddr = 0;
    // TWIbufIndex = 0;
    // TWIbufLength = 16;
    // twiSendStart(); //Send Start Condition
}

int8_t getTemperature(void)
{
    return TWIbuf[0x11];    // Here's the MSB
}

uint8_t decToBcd(uint8_t val)
{
// Convert normal decimal numbers to binary coded decimal
    return ((val/10*16) + (val%10));
}

uint8_t bcdToDec(uint8_t val)
{
// Convert binary coded decimal to normal decimal numbers
    return ((val/16*10) + (val%16));
}

void DS3231_init(void)
{
    TWI_init();

    EICRA |= 1 << ISC01; // The falling edge of INT0 generates an interrupt request
    EIMSK |= 1 << INT0; // External Interrupt Request 0 Enable
}

void readRTC(void)
{
    while (TWI_master_receive(DS3231_ADDRESS, 0, TWIbuf, 18));
}

SIGNAL(INT0_vect)
{
    RTC_int_flag = 1;
}
