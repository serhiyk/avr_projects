#ifndef DS3231_h
#define DS3231_h

#include <stdint.h>

void ds3231_init(void);

// the get*() functions retrieve current values of the registers.
uint8_t get_second(void);
uint8_t get_second_bcd(void);
uint8_t get_minute(void);
uint8_t get_minute_bcd(void);
uint8_t get_hour(void);
uint8_t get_hour_bcd(void);
uint8_t get_dow(void);
uint8_t get_date(void);
uint8_t get_date_bcd(void);
uint8_t get_month(void);
uint8_t get_month_bcd(void);
uint8_t get_year(void);
// Last 2 digits only

void set_time(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dow, uint8_t date, uint8_t month, uint8_t year);
void set_time_bcd(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dow, uint8_t date, uint8_t month, uint8_t year);

// Temperature function
int8_t get_temperature(void);

// Alarm functions

/* Retrieves everything you could want to know about alarm
 * one.
 * A1Dy true makes the alarm go on A1Day = Day of Week,
 * A1Dy false makes the alarm go on A1Day = Date of month.
 *
 * byte AlarmBits sets the behavior of the alarms:
 *  Dy  A1M4    A1M3    A1M2    A1M1    Rate
 *  X   1       1       1       1       Once per second
 *  X   1       1       1       0       Alarm when seconds match
 *  X   1       1       0       0       Alarm when min, sec match
 *  X   1       0       0       0       Alarm when hour, min, sec match
 *  0   0       0       0       0       Alarm when date, h, m, s match
 *  1   0       0       0       0       Alarm when DoW, h, m, s match
 *
 *  Dy  A2M4    A2M3    A2M2    Rate
 *  X   1       1       1       Once per minute (at seconds = 00)
 *  X   1       1       0       Alarm when minutes match
 *  X   1       0       0       Alarm when hours and minutes match
 *  0   0       0       0       Alarm when date, hour, min match
 *  1   0       0       0       Alarm when DoW, hour, min match
 */

void ds3231_read(void);

typedef void (*ds3231_data_ready_cb)(void);
void ds3231_handler(ds3231_data_ready_cb callback);

#endif
