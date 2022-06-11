#ifndef WS2812B_H
#define WS2812B_H

#include <stdint.h>

typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} ws2812b_data_t;

void ws2812b_init(ws2812b_data_t *buf, int leds);
void ws2812b_send(void);

#endif  /* WS2812B_H */
