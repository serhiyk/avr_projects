#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_activate(void);
void display_deactivate(void);
void time_update_handler(void);
void print_ext_temperature(int16_t temperature);
void clear_ext_temperature(void);

#endif	/* DISPLAY_H */