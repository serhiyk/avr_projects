#ifndef TWI_H_INCLUDED
#define TWI_H_INCLUDED

#include <stdint.h>

void TWI_Init(void);
void twiMasterSend(uint8_t Addr, uint8_t data);
uint8_t twiMasterReceive(uint8_t Addr);
void twitmp(void);

#endif
