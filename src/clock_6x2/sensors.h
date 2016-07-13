#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

void sensors_init(void);
uint16_t sensors_get_temperature(void);

#endif /* SENSORS_H */
