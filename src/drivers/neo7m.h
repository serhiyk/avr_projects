#ifndef NEO7M_H
#define NEO7M_H

#include <stdint.h>

typedef void (*time_cb)(void);

void neo7m_init(time_cb cb);
void neo7m_handler(void);
uint8_t get_second_bcd_l(void);
uint8_t get_second_bcd_h(void);
uint8_t get_minute_bcd_l(void);
uint8_t get_minute_bcd_h(void);
uint8_t get_hour_bcd_l(void);
uint8_t get_hour_bcd_h(void);
uint8_t get_dow(void);
uint8_t get_date_bcd_l(void);
uint8_t get_date_bcd_h(void);
uint8_t get_month(void);
uint8_t get_year(void);

#endif
