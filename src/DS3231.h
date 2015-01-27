#ifndef DS3231_h
#define DS3231_h

#include <stdint.h>

void TWI_Init(void);

extern volatile uint8_t RTC_int_flag;

void ClearAlarm(void);
void readRTC(void);

// the get*() functions retrieve current values of the registers.
uint8_t getSecond(void); 
uint8_t getSecondBCD(void); 
uint8_t getMinute(void);
uint8_t getMinuteBCD(void);
uint8_t getHour(void);
uint8_t getHourBCD(void);
uint8_t getDoW(void); 
uint8_t getDate(void);
uint8_t getDateBCD(void);
uint8_t getMonth(void);
uint8_t getYear(void); 
// Last 2 digits only

void setTime(uint8_t Second, uint8_t Minute, uint8_t Hour, uint8_t DoW, uint8_t Date, uint8_t Month, uint8_t Year);

// Temperature function
int8_t getTemperature(void); 

// Alarm functions

/* Retrieves everything you could want to know about alarm
 * one. 
 * A1Dy true makes the alarm go on A1Day = Day of Week,
 * A1Dy false makes the alarm go on A1Day = Date of month.
 *
 * byte AlarmBits sets the behavior of the alarms:
 *	Dy	A1M4	A1M3	A1M2	A1M1	Rate
 *	X	1		1		1		1		Once per second
 *	X	1		1		1		0		Alarm when seconds match
 *	X	1		1		0		0		Alarm when min, sec match
 *	X	1		0		0		0		Alarm when hour, min, sec match
 *	0	0		0		0		0		Alarm when date, h, m, s match
 *	1	0		0		0		0		Alarm when DoW, h, m, s match
 *
 *	Dy	A2M4	A2M3	A2M2	Rate
 *	X	1		1		1		Once per minute (at seconds = 00)
 *	X	1		1		0		Alarm when minutes match
 *	X	1		0		0		Alarm when hours and minutes match
 *	0	0		0		0		Alarm when date, hour, min match
 *	1	0		0		0		Alarm when DoW, hour, min match
 */

uint8_t decToBcd(uint8_t val); 
	// Convert normal decimal numbers to binary coded decimal
uint8_t bcdToDec(uint8_t val); 
	// Convert binary coded decimal to normal decimal numbers

#endif
