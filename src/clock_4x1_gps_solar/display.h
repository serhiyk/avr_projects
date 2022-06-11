#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h>

void display_init(void);
void time_update_handler(uint8_t h_h, uint8_t h_l, uint8_t m_h, uint8_t m_l);
void print_ext_temperature(int16_t temperature);
void print_screensaver(void);

#endif	/* DISPLAY_H */
