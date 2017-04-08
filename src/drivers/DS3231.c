#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "TWI.h"
#include "serial.h"
#include "DS3231.h"

#define COMMAND_ID_SET_TIME 0

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

#define DST_EEPROM_ADDRESS 1

static uint8_t tmp_buf[2];
static uint8_t ds3231_buf[19];
static uint8_t dst_flag;
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
    return (bcd_to_dec(ds3231_buf[DS3231_REG_MONTH_CENTURY] & 0b01111111));
}

uint8_t get_month_bcd(void)
{
    return ds3231_buf[DS3231_REG_MONTH_CENTURY] & 0b01111111;
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

void command_set_time(uint8_t *cmd, uint8_t len)
{
    if (len != 15)
    {
        return;
    }
    for (uint8_t i=1; i<15; i++)
    {
        if (cmd[i] < '0' || cmd[i] > '9')
        {
            return;
        }
        cmd[i] -= '0';
    }
    set_time_bcd((cmd[1] << 4) | cmd[2],
                 (cmd[3] << 4) | cmd[4],
                 (cmd[5] << 4) | cmd[6],
                 (cmd[7] << 4) | cmd[8],
                 (cmd[9] << 4) | cmd[10],
                 (cmd[11] << 4) | cmd[12],
                 (cmd[13] << 4) | cmd[14]);
}

static inline void correct_dst(void)
{
    // TODO: correct date
    uint8_t month = get_month_bcd();
    uint8_t dow = get_dow();
    uint8_t date = get_date();
    uint8_t hour = get_hour();
    if (dst_flag == 0) // Winter time
    {
        if (month < 0x03 || month > 0x10) // it is Winter
        {
            return;
        }
        if (month == 0x03) // March
        {
            if (23 + dow >= date) // before last Sunday
            {
                return;
            }
            if (dow == 1 && hour < 3) // Sunday before 3
            {
                return;
            }
        }
        else if (month == 0x10) // October
        {
            if (23 + dow < date) // last Sunday or later
            {
                if (dow == 1) // Sunday
                {
                    if (hour >= 3) // after 3
                    {
                        return;
                    }
                }
                else
                {
                    return;
                }
            }
        }
        if (hour < 23)
        {
            hour++;
        }
        else
        {
            hour = 0;
        }
        dst_flag = 1;
    }
    else if (dst_flag == 1) // Summer time
    {
        if (month > 0x03 && month < 0x10) // it is Summer
        {
            return;
        }
        if (month == 0x10) // October
        {
            if (23 + dow >= date) // before last Sunday
            {
                return;
            }
            if (dow == 1 && hour <= 3) // Sunday before 3
            {
                return;
            }
        }
        else if (month == 0x03) // March
        {
            if (23 + dow < date) // last Sunday or later
            {
                if (dow == 1) // Sunday
                {
                    if (hour > 3) // after 4
                    {
                        return;
                    }
                }
                else
                {
                    return;
                }
            }
        }
        if (hour > 0)
        {
            hour--;
        }
        else
        {
            hour = 23;
        }
        dst_flag = 0;
    }
    else // init state
    {
        if (month < 0x03 || month > 0x10 || (month == 0x03 && (23 + dow >= date)) || (month == 0x10 && (23 + dow < date)))
        {
            dst_flag = 0;
        }
        else
        {
            dst_flag = 1;
        }
    }
    tmp_buf[0] = DS3231_REG_HOURS;
    tmp_buf[1] = dec_to_bcd(hour);
    twi_master_send(DS3231_ADDRESS, tmp_buf, 2, 0);
    eeprom_write_byte((uint8_t*) DST_EEPROM_ADDRESS, dst_flag);
}

static inline void check_dst(void)
{
    uint8_t month = get_month_bcd();
    uint8_t dow = get_dow();
    uint8_t date = get_date_bcd();
    uint8_t hour = get_hour_bcd();
    if (dst_flag == 0) // Winter time
    {
        if (month == 0x03) // March
        {
            if (dow == 1) // Sunday
            {
                if (date > 0x24) // last Sunday
                {
                    if (hour == 0x03)
                    {
                        tmp_buf[0] = DS3231_REG_HOURS;
                        tmp_buf[1] = 4;
                        if (twi_master_send(DS3231_ADDRESS, tmp_buf, 2, 0) != 0)
                        {
                            return;
                        }
                        dst_flag = 1;
                        eeprom_write_byte((uint8_t*) DST_EEPROM_ADDRESS, dst_flag);
                    }
                }
            }
        }
    }
    else // Summer time
    {
        if (month == 0x10) // October
        {
            if (dow == 1) // Sunday
            {
                if (date > 0x24) // last Sunday
                {
                    if (hour == 0x04)
                    {
                        tmp_buf[0] = DS3231_REG_HOURS;
                        tmp_buf[1] = 3;
                        if (twi_master_send(DS3231_ADDRESS, tmp_buf, 2, 0) != 0)
                        {
                            return;
                        }
                        dst_flag = 0;
                        eeprom_write_byte((uint8_t*) DST_EEPROM_ADDRESS, dst_flag);
                    }
                }
            }
        }
    }
}

void ds3231_init(void)
{
    while (!twi_ready());
    tmp_buf[0] = DS3231_REG_SECONDS;
    if (twi_master_transfer(DS3231_ADDRESS, tmp_buf, ds3231_buf, 1, 18, data_ready_cb) != 0)
    {
        return;
    }
    while (data_ready_flag == 0);
    dst_flag = eeprom_read_byte((uint8_t*) DST_EEPROM_ADDRESS);
    correct_dst();
    EICRA |= 1 << ISC01; // The falling edge of INT0 generates an interrupt request
    EIMSK |= 1 << INT0; // External Interrupt Request 0 Enable
    serial_register_command(COMMAND_ID_SET_TIME, command_set_time);
}

SIGNAL(INT0_vect)
{
    rtc_int_flag = 1;
}

void ds3231_read(void)
{
    rtc_int_flag = 1;
}

void ds3231_handler(ds3231_data_ready_cb callback)
{
    if (!twi_ready())
    {
        return;
    }
    if (rtc_int_flag)
    {
        tmp_buf[0] = DS3231_REG_CONTROL_STATUS;
        tmp_buf[1] = 0x00;
        if (twi_master_send(DS3231_ADDRESS, tmp_buf, 2, clear_alarm_cb) == 0)
        {
            rtc_int_flag = 0;
        }
    }
    else if (clear_alarm_flag)
    {
        tmp_buf[0] = DS3231_REG_SECONDS;
        if (twi_master_transfer(DS3231_ADDRESS, tmp_buf, ds3231_buf, 1, 18, data_ready_cb) == 0)
        {
            clear_alarm_flag = 0;
        }
    }
    else if (data_ready_flag)
    {
        data_ready_flag = 0;
        check_dst();
        if (callback)
        {
            callback();
        }
    }
}
