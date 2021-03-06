#include <avr/pgmspace.h>
#include "fonts.h"

#define ASCII_BEGIN    0x20
#define ASCII_END      0x7D

const uint8_t TripleDotDigital15table[][30] PROGMEM =
{
  /* character 0x30 ('0'): (width=11) */
0x7f,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0xe1,0xe0,
0xe3,0xe0,
0xe7,0xe0,
0xee,0xe0,
0xfc,0xe0,
0xf8,0xe0,
0xf0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xc0,

  /* character 0x31 ('1'): (width=11) */
0xfe,0x0,
0xfe,0x0,
0xfe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xe,0x0,
0xff,0xe0,
0xff,0xe0,
0xff,0xe0,

  /* character 0x32 ('2'): (width=11) */
0x7f,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x7f,0xe0,
0xff,0xe0,
0xff,0xc0,
0xe0,0x0,
0xe0,0x0,
0xe0,0x0,
0xff,0xe0,
0xff,0xe0,
0xff,0xe0,

  /* character 0x33 ('3'): (width=11) */
0x7f,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x1f,0xe0,
0x1f,0xc0,
0x1f,0xe0,
0x0,0xe0,
0x0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xc0,

  /* character 0x34 ('4'): (width=11) */
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0xff,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,

  /* character 0x35 ('5'): (width=11) */
0xff,0xe0,
0xff,0xe0,
0xff,0xe0,
0xe0,0x0,
0xe0,0x0,
0xe0,0x0,
0xff,0xc0,
0xff,0xe0,
0xff,0xe0,
0x0,0xe0,
0x0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xc0,

  /* character 0x36 ('6'): (width=11) */
0x7f,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0xe0,0x0,
0xe0,0x0,
0xff,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xc0,

  /* character 0x37 ('7'): (width=11) */
0xff,0xe0,
0xff,0xe0,
0xff,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,
0x0,0xe0,

  /* character 0x38 ('8'): (width=11) */
0x7f,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0x7f,0xc0,
0xff,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xc0,

  /* character 0x39 ('9'): (width=11) */
0x7f,0xc0,
0xff,0xe0,
0xff,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xe0,
0x0,0xe0,
0x0,0xe0,
0xe0,0xe0,
0xff,0xe0,
0xff,0xe0,
0x7f,0xc0,

};

const uint8_t DotDigital7table[][7] PROGMEM =
{
  /* character 0x30 ('0'): (width=5) */
0x70,
0x88,
0x98,
0xa8,
0xc8,
0x88,
0x70,

  /* character 0x31 ('1'): (width=5) */
0x20,
0x60,
0xa0,
0x20,
0x20,
0x20,
0xf8,

  /* character 0x32 ('2'): (width=5) */
0x70,
0x88,
0x8,
0x30,
0x40,
0x80,
0xf8,

  /* character 0x33 ('3'): (width=5) */
0x70,
0x88,
0x8,
0x30,
0x8,
0x88,
0x70,

  /* character 0x34 ('4'): (width=5) */
0x30,
0x50,
0x90,
0xf8,
0x10,
0x10,
0x10,

  /* character 0x35 ('5'): (width=5) */
0xf8,
0x80,
0x80,
0xf0,
0x8,
0x8,
0xf0,

  /* character 0x36 ('6'): (width=5) */
0x70,
0x88,
0x80,
0xf0,
0x88,
0x88,
0x70,

  /* character 0x37 ('7'): (width=5) */
0xf8,
0x88,
0x8,
0x10,
0x20,
0x20,
0x20,

  /* character 0x38 ('8'): (width=5) */
0x70,
0x88,
0x88,
0x70,
0x88,
0x88,
0x70,

  /* character 0x39 ('9'): (width=5) */
0x70,
0x88,
0x88,
0x78,
0x8,
0x88,
0x70,

};

