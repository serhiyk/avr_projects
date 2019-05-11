#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_activate(void);
void display_deactivate(void);
void display_scroll_up(void);
void time_update_handler(void);
void print_ext_temperature(int16_t temperature);
void clear_ext_temperature(void);
void print_pressure(int32_t pressure);
void print_int_temperature(int16_t t);

#endif	/* DISPLAY_H */
