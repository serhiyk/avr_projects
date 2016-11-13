#ifndef BMP180_h
#define BMP180_h

#include <stdint.h>

typedef void (*bmp180_temperature_cb)(int16_t temperature);
typedef void (*bmp180_pressure_cb)(int32_t pressure);

void bmp180_init(bmp180_temperature_cb t_cb, bmp180_pressure_cb p_cb);
void bmp180_handler(void);

#endif
