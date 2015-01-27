#ifndef CYRILLICFONT_H
#define CYRILLICFONT_H

#include <avr/pgmspace.h> 

extern const uint8_t cyrillic7table[256][7];
extern const uint8_t cyrillic7width[256];
extern const uint8_t cyrillic5table[256][5];
extern const uint8_t cyrillic5width[256];

#define cyr7table(c,l) pgm_read_byte(&cyrillic7table[c][l])
#define cyr7width(c) pgm_read_byte(&cyrillic7width[c])
#define cyr5table(c,l) pgm_read_byte(&cyrillic5table[c][l])
#define cyr5width(c) pgm_read_byte(&cyrillic5width[c])

#endif /*CYRILLICFONT_H*/
