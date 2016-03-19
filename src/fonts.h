#ifndef FONTS_H
#define FONTS_H

extern const uint8_t TripleDotDigital15table[][30];
extern const uint8_t DotDigital7table[][7];

#define tdDigital15table(c,l) pgm_read_byte(&TripleDotDigital15table[c][l])
#define dDigital7table(c,l) pgm_read_byte(&DotDigital7table[c][l])

#endif /*FONTS_H*/
