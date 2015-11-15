#ifndef TWI_H
#define TWI_H

#include <stdint.h>

void TWI_init(void);
void twiMasterSend(uint8_t Addr, uint8_t data);
uint8_t TWI_master_receive(uint8_t dev_addr, uint8_t addr, uint8_t *buf, uint8_t len);
uint8_t TWI_master_send(uint8_t dev_addr, uint8_t addr, uint8_t *buf, uint8_t len);

#endif /* TWI_H */
