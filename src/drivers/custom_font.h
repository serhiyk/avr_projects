#ifndef CUSTOM_FONT_H
#define CUSTOM_FONT_H

#include <stdint.h>

typedef struct  __attribute((packed))
{
    uint16_t shift;
    uint8_t width;
} custom_font_char_shift_t;

typedef struct
{
    uint8_t char_start;
    uint8_t size;
    uint8_t height;
    const uint8_t* char_table;
    const custom_font_char_shift_t* shift_table;
} custom_font_t;

#endif
