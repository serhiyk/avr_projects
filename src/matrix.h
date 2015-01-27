#ifndef MATRIX_H
#define	MATRIX_H
#include <stdint.h>

void SPI_MasterInit(void);
void Matrix_Init(void);
void PrintClockToLCD(uint8_t h, uint8_t m);
void VerticalScrollLCD(void);
void HorisontalScrollLCD(void);
void LoadLCDBuffer(void);
void MatrixUpdateHandler(void);
void TimeUpdateHandler(void);

#endif	/* MATRIX_H */
