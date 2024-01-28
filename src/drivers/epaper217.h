#ifndef EPAPER217_H
#define EPAPER217_H

#include <stdint.h>

// 122 x 250
#define EPD_WIDTH       (122)
#define EPD_HEIGHT      (250)

void epaper_init(void);
void epaper_display_frame(void);
void epaper_update_frame_buf(uint8_t* frame_buffer, uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red);
void epaper_update_frame_progmem(const unsigned char* frame_buffer, uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red);
void epaper_clear_frame(uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red);
void epaper_clear_frame_all(uint8_t is_red);
void epaper_fill_frame(uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red);
void epaper_fill_frame_all(uint8_t is_red);
void epaper_sleep(void);
void epaper_wake(void);

#endif  /* EPAPER217_H */
