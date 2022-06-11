#ifndef ONEWIRE_BLOCKING_H
#define ONEWIRE_BLOCKING_H

#include <stdint.h>

uint8_t onewire_master_reset(void);
void onewire_master_write(uint8_t data);
uint8_t onewire_master_read(void);

#endif  /* ONEWIRE_BLOCKING_H */
