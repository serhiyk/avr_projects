#ifndef MONTHS_H
#define MONTHS_H

extern const uint8_t months_table[][5][4];
extern const uint8_t months_width[];

#define _months_table(m,r,c) pgm_read_byte(&months_table[m][r][c])
#define _months_width(c) pgm_read_byte(&months_width[c])

#endif /*MONTHS_H*/
