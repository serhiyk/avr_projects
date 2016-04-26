#include "text.h"

const uint8_t emp[] PROGMEM = "";
const uint8_t sun[] PROGMEM = "�����";
const uint8_t mon[] PROGMEM = "��������";
const uint8_t tue[] PROGMEM = "³������";
const uint8_t wed[] PROGMEM = "������";
const uint8_t thu[] PROGMEM = "������";
const uint8_t fri[] PROGMEM = "�'������";
const uint8_t sat[] PROGMEM = "������";
const uint8_t* dow_str[] =
{
    emp, sun, mon, tue, wed, thu, fri, sat
};
const uint8_t dow_len[] PROGMEM =
{
    0, 6, 9, 8, 6, 6, 8, 6
};
const uint8_t dow_shift[] PROGMEM =
{
    0, 8, 0, 4, 7, 8, 2, 6
};

const uint8_t months_table[][5][4] PROGMEM =
{
/* January: (width=21) */
0x65,0x29,0x38,0x0,
0x91,0x29,0x48,0x0,
0x84,0xef,0x38,0x0,
0x94,0x29,0x28,0x0,
0x64,0x29,0x48,0x0,

/* February: (width=29) */
0x34,0xce,0x67,0x30,
0x55,0x24,0x94,0x48,
0x57,0x24,0x94,0x48,
0x55,0x24,0x94,0x48,
0x94,0xc4,0x64,0x30,

/* March: (width=31) */
0xf7,0x73,0xb2,0x4e,
0x84,0x4a,0xa,0x52,
0xe7,0x73,0xb3,0xce,
0x94,0x42,0xa,0x4a,
0xe7,0x43,0xb2,0x52,

/* April: (width=25) */
0x97,0x2e,0x93,0x80,
0xa4,0x84,0x94,0x80,
0xc7,0x24,0xf3,0x80,
0xa4,0xa4,0x92,0x80,
0x97,0x24,0x94,0x80,

/* May: (width=28) */
0xee,0x33,0x92,0x70,
0x49,0x4a,0x52,0x90,
0x4e,0x4b,0x9e,0x70,
0x48,0x7a,0x52,0x50,
0x48,0x4b,0x92,0x90,

/* June: (width=28) */
0x97,0x73,0x92,0x70,
0x94,0x4a,0x52,0x90,
0x77,0x73,0x9e,0x70,
0x14,0x42,0x52,0x50,
0x17,0x43,0x92,0x90,

/* July: (width=24) */
0x34,0xbd,0x27,0x0,
0x54,0xa5,0x29,0x0,
0x55,0xa5,0xe7,0x0,
0x56,0xa5,0x25,0x0,
0x94,0xa5,0x29,0x0,

/* August: (width=28) */
0x67,0x73,0xd2,0x70,
0x94,0x4a,0x52,0x90,
0x87,0x72,0x5e,0x70,
0x94,0x42,0x52,0x50,
0x67,0x42,0x52,0x90,

/* September: (width=32) */
0xe7,0x73,0x99,0x27,
0x94,0x4a,0x25,0x29,
0xe7,0x73,0xa1,0xe7,
0x94,0x42,0x25,0x25,
0xe7,0x43,0x99,0x29,

/* October: (width=29) */
0xa9,0x9c,0xe9,0x38,
0xaa,0x52,0x49,0x48,
0x72,0x5c,0x4f,0x38,
0xaa,0x52,0x49,0x28,
0xa9,0x9c,0x49,0x48,

/* November: (width=30) */
0x34,0x99,0xcc,0xf0,
0x54,0xa4,0x92,0x90,
0x55,0xa0,0x92,0x90,
0x56,0xa4,0x92,0x90,
0x94,0x98,0x8c,0x94,

/* December: (width=27) */
0xee,0x51,0xa4,0xe0,
0x89,0x52,0xa5,0x20,
0x8e,0x32,0xbc,0xe0,
0x88,0x17,0xa4,0xa0,
0x88,0x64,0xa5,0x20,

};

const uint8_t months_width[] PROGMEM =
{
/*    width     month     */
/*    =====    ========== */
        21, /*  January   */
        29, /*  February  */
        31, /*  March     */
        25, /*  April     */
        28, /*  May       */
        28, /*  June      */
        24, /*  July      */
        28, /*  August    */
        32, /*  September */
        29, /*  October   */
        30, /*  November  */
        27, /*  December  */
};

#ifdef UNUSED
const uint8_t jan[] PROGMEM = "����";
const uint8_t feb[] PROGMEM = "������";
const uint8_t mar[] PROGMEM = "�������";
const uint8_t apr[] PROGMEM = "�����";
const uint8_t may[] PROGMEM = "������";
const uint8_t jun[] PROGMEM = "������";
const uint8_t jul[] PROGMEM = "�����";
const uint8_t aug[] PROGMEM = "������";
const uint8_t sep[] PROGMEM = "�������";
const uint8_t oct[] PROGMEM = "������";
const uint8_t nov[] PROGMEM = "���������";
const uint8_t dec[] PROGMEM = "������";
const uint8_t *mon_str[] =
{
    emp, jan, feb, mar, apr, may, jun, jul, aug, sep, oct, nov, dec
};
#define get_mon(d,n) pgm_read_byte(&mon_str[d][n])

const uint8_t mon_len[] PROGMEM =
{
    0, 5, 6, 7, 6, 6, 6, 5, 6, 7, 6, 9, 6
};
#define get_mon_len(d) pgm_read_byte(&mon_len[d])

const uint8_t mon_shift[] PROGMEM =
{
    0, 0, 0, 0, 0, 0, 6, 8, 6, 0, 0, 0, 0
};
#define get_mon_shift(d) pgm_read_byte(&mon_shift[d])
#endif