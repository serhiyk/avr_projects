#ifndef TIME_ASYNC_H
#define TIME_ASYNC_H

#include <stdint.h>

typedef void (*time_async_cb)(void);
void time_async_init(time_async_cb cb);
void time_async_set(
    uint8_t second,
    uint8_t minute,
    uint8_t hour,
    uint8_t dow,
    uint8_t date,
    uint8_t month,
    uint16_t year);
void time_async_get_hms(uint8_t *h, uint8_t *m, uint8_t *s);

#endif  /* TIME_ASYNC_H */
