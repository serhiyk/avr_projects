#ifndef TEXT_H
#define TEXT_H

#include <avr/pgmspace.h>

extern const uint8_t* dow_str[];
extern const uint8_t dow_len[];
extern const uint8_t dow_shift[];
#define get_dow_str(d,n) pgm_read_byte(&dow_str[d][n])
#define get_dow_len(d) pgm_read_byte(&dow_len[d])
#define get_dow_shift(d) pgm_read_byte(&dow_shift[d])

extern const uint8_t months_table[][5][4];
extern const uint8_t months_width[];

#define _months_table(m,r,c) pgm_read_byte(&months_table[m][r][c])
#define _months_width(c) pgm_read_byte(&months_width[c])

#endif /*TEXT_H*/
