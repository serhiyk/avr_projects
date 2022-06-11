#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h>

void display_init(void);
void time_update_handler(void);
void print_ext_temperature(int16_t temperature);

#endif	/* DISPLAY_H */
