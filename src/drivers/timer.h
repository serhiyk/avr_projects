#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

typedef void (*timer_cb)(void);
void timer_init(void);
uint8_t timer_register(uint8_t timer, uint8_t timeout, timer_cb callback);
uint8_t timer_stop(uint8_t timer);
uint8_t timer_start(uint8_t timer, uint8_t timeout);
void timer_handler(void);

#endif  /* TIMER_H */
