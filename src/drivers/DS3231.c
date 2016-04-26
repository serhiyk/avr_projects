#include <avr/io.h>
#include <avr/interrupt.h>
#include "TWI.h"
#include "DS3231.h"

#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_MINUTES 0x01
#define DS3231_REG_HOURS 0x02
#define DS3231_REG_DAY 0x03
#define DS3231_REG_DATE 0x04
#define DS3231_REG_MONTH_CENTURY 0x05
#define DS3231_REG_YEAR 0x06
#define DS3231_REG_ALARM_1_SECONDS 0x07
#define DS3231_REG_ALARM_1_MINUTES 0x08
#define DS3231_REG_ALARM_1_HOURS 0x09
#define DS3231_REG_ALARM_1_DAY_DATE 0x0A
#define DS3231_REG_ALARM_2_MINUTES 0x0B
#define DS3231_REG_ALARM_2_HOURS 0x0C
#define DS3231_REG_ALARM_2_DAY_DATE 0x0D
#define DS3231_REG_CONTROL 0x0E
#define DS3231_REG_CONTROL_STATUS 0x0F
#define DS3231_REG_AGING_OFFSET 0x10
#define DS3231_REG_TEMP_MSB 0x11
#define DS3231_REG_TEMP_LSB 0x12

#define DS3231_ADDRESS 0xD0

static uint8_t ds3231_buf[19];

volatile uint8_t rtc_int_flag=1;
volatile uint8_t clear_alarm_flag=0;
volatile uint8_t data_ready_flag=0;

uint8_t dec_to_bcd(uint8_t val)
{
// Convert normal decimal numbers to binary coded decimal
    return ((val/10*16) + (val%10));
}

uint8_t bcd_to_dec(uint8_t val)
{
// Convert binary coded decimal to normal decimal numbers
    return ((val/16*10) + (val%16));
}

void clear_alarm_cb(void)
{
    clear_alarm_flag = 1;
}

void data_ready_cb(void)
{
    data_ready_flag = 1;
}

uint8_t get_second(void)
{
    return bcd_to_dec(ds3231_buf[DS3231_REG_SECONDS]);
}

uint8_t get_second_bcd(void)
{
    return ds3231_buf[DS3231_REG_SECONDS];
}

uint8_t get_minute(void)
{
    return bcd_to_dec(ds3231_buf[DS3231_REG_MINUTES]);
}

uint8_t get_minute_bcd(void)
{
    return ds3231_buf[DS3231_REG_MINUTES];
}

uint8_t get_hour(void)
{
    uint8_t temp_buffer;
    uint8_t hour;
    temp_buffer = ds3231_buf[DS3231_REG_HOURS];
    if(temp_buffer & 0b01000000)
        hour = bcd_to_dec(temp_buffer & 0b00011111);
    else
        hour = bcd_to_dec(temp_buffer & 0b00111111);
    return hour;
}

uint8_t get_hour_bcd(void)
{
    return ds3231_buf[DS3231_REG_HOURS];
}

uint8_t get_dow(void)
{
    return bcd_to_dec(ds3231_buf[DS3231_REG_DAY]);
}

uint8_t get_date(void)
{
    return bcd_to_dec(ds3231_buf[DS3231_REG_DATE]);
}

uint8_t get_date_bcd(void)
{
    return ds3231_buf[DS3231_REG_DATE];
}

uint8_t get_month(void)
{
    return (bcd_to_dec(ds3231_buf[DS3231_REG_MONTH_CENTURY] & 0b01111111)) ;
}

uint8_t get_year(void)
{
    return bcd_to_dec(ds3231_buf[DS3231_REG_YEAR]);
}

void set_time(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dow, uint8_t date, uint8_t month, uint8_t year)
{
    while (!twi_ready());
    ds3231_buf[0] = DS3231_REG_SECONDS;
    ds3231_buf[1] = dec_to_bcd(second);
    ds3231_buf[2] = dec_to_bcd(minute);
    ds3231_buf[3] = dec_to_bcd(hour);// & 0b10111111;
    ds3231_buf[4] = dec_to_bcd(dow);
    ds3231_buf[5] = dec_to_bcd(date);
    ds3231_buf[6] = dec_to_bcd(month);
    ds3231_buf[7] = dec_to_bcd(year);
    ds3231_buf[8] = 0x80;
    ds3231_buf[9] = 0x80;
    ds3231_buf[10] = 0x80;
    ds3231_buf[11] = 0x80;
    ds3231_buf[12] = 0x80;
    ds3231_buf[13] = 0x80;
    ds3231_buf[14] = 0x80;
    ds3231_buf[15] = 0x05; // Alarm 1 Interrupt Enable
    ds3231_buf[16] = 0x00;
    twi_master_send(DS3231_ADDRESS, ds3231_buf, 17, 0);
}

void set_time_bcd(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dow, uint8_t date, uint8_t month, uint8_t year)
{
    while (!twi_ready());
    ds3231_buf[0] = DS3231_REG_SECONDS;
    ds3231_buf[1] = second;
    ds3231_buf[2] = minute;
    ds3231_buf[3] = hour;// & 0b10111111;
    ds3231_buf[4] = dow;
    ds3231_buf[5] = date;
    ds3231_buf[6] = month;
    ds3231_buf[7] = year;
    twi_master_send(DS3231_ADDRESS, ds3231_buf, 8, 0);
}

int8_t get_temperature(void)
{
    return ds3231_buf[DS3231_REG_TEMP_MSB];    // Here's the MSB
}

void ds3231_init(void)
{
    twi_init();

    EICRA |= 1 << ISC01; // The falling edge of INT0 generates an interrupt request
    EIMSK |= 1 << INT0; // External Interrupt Request 0 Enable
}

SIGNAL(INT0_vect)
{
    rtc_int_flag = 1;
}

void ds3231_handler(ds3231_data_ready_cb callback)
{
    if (rtc_int_flag)
    {
        if (twi_ready())
        {
            static uint8_t clear_alarm_buf[] = {DS3231_REG_CONTROL_STATUS, 0x00};
            if (twi_master_send(DS3231_ADDRESS, clear_alarm_buf, sizeof(clear_alarm_buf), clear_alarm_cb) == 0)
            {
                rtc_int_flag = 0;
            }
        }
    }
    if (clear_alarm_flag)
    {
        if (twi_ready())
        {
            static uint8_t read_rtc_buf[] = {DS3231_REG_SECONDS};
            if (twi_master_transfer(DS3231_ADDRESS, read_rtc_buf, ds3231_buf, sizeof(read_rtc_buf), 18, data_ready_cb) == 0)
            {
                clear_alarm_flag = 0;
            }
        }
    }
    if (data_ready_flag)
    {
        data_ready_flag = 0;
        if (callback)
        {
            callback();
        }
    }
}
