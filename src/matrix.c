#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "max7219.h"
#include "Fonts.h"
#include "cyrillicfont.h"
#include "months.h"
#include "UART.h"
#include "DS3231.h"
#include "matrix.h"

#define max7219InUse 12
#define max7219PerRow 6
#define max7219Rows 8

const uint8_t BitReverseTable256[] PROGMEM =
{
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};
#define reverse(byte) pgm_read_byte(&BitReverseTable256[byte])

const uint8_t emp[] PROGMEM = "";
const uint8_t sun[] PROGMEM = "Неділя";
const uint8_t mon[] PROGMEM = "Понеділок";
const uint8_t tue[] PROGMEM = "Вівторок";
const uint8_t wed[] PROGMEM = "Середа";
const uint8_t thu[] PROGMEM = "Четвер";
const uint8_t fri[] PROGMEM = "П'ятниця";
const uint8_t sat[] PROGMEM = "Субота";
const uint8_t *dow_str[] =
{
	emp, sun, mon, tue, wed, thu, fri, sat
};
#define get_dow_str(d,n) pgm_read_byte(&dow_str[d][n])

const uint8_t dow_len[] PROGMEM =
{
	0, 6, 9, 8, 6, 6, 8, 6
};
#define get_dow_len(d) pgm_read_byte(&dow_len[d])

const uint8_t dow_shift[] PROGMEM =
{
	0, 8, 0, 4, 7, 8, 2, 6
};
#define get_dow_shift(d) pgm_read_byte(&dow_shift[d])

const uint8_t jan[] PROGMEM = "січня";
const uint8_t feb[] PROGMEM = "лютого";
const uint8_t mar[] PROGMEM = "березня";
const uint8_t apr[] PROGMEM = "квітня";
const uint8_t may[] PROGMEM = "травня";
const uint8_t jun[] PROGMEM = "червня";
const uint8_t jul[] PROGMEM = "липня";
const uint8_t aug[] PROGMEM = "серпня";
const uint8_t sep[] PROGMEM = "вересня";
const uint8_t oct[] PROGMEM = "жовтня";
const uint8_t nov[] PROGMEM = "листопада";
const uint8_t dec[] PROGMEM = "грудня";
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

#define LCD_BUF_ROWS 32

static uint8_t LCD1_buf[max7219Rows*2][max7219PerRow];
static uint8_t LCD2_buf[max7219Rows][max7219PerRow];
static uint8_t LCD3_buf[max7219Rows][32];
static uint8_t *LCD_buf[LCD_BUF_ROWS] =
{
	LCD1_buf[0], LCD1_buf[1], LCD1_buf[2], LCD1_buf[3], LCD1_buf[4], LCD1_buf[5], LCD1_buf[6], LCD1_buf[7],
	LCD1_buf[8], LCD1_buf[9], LCD1_buf[10], LCD1_buf[11], LCD1_buf[12], LCD1_buf[13], LCD1_buf[14], LCD1_buf[15],
	LCD2_buf[0], LCD2_buf[1], LCD2_buf[2], LCD2_buf[3], LCD2_buf[4], LCD2_buf[5], LCD2_buf[6], LCD2_buf[7],
	LCD3_buf[0], LCD3_buf[1], LCD3_buf[2], LCD3_buf[3], LCD3_buf[4], LCD3_buf[5], LCD3_buf[6], LCD3_buf[7]
};
//#define LCD_buf_dow (&LCD3_buf[1])

static volatile uint8_t LCD_buf_Row_Shift=0;
static volatile uint8_t LCD_buf_Col_Shift=0;
static volatile uint8_t SPI_buf[256];
static volatile uint8_t SPI_buf_Head;
static volatile uint8_t SPI_buf_Tail;
static volatile uint8_t SPI_buf_Counter;
static volatile uint8_t SPI_flag;

static volatile uint8_t LCD_update;

#define LCD_STATE_UP (1<<0)
#define LCD_STATE_GOTO_UP (1<<1)
#define LCD_STATE_DOWN (1<<2)
#define LCD_STATE_GOTO_DOWN (1<<3)
#define LCD_STATE_GOTO_RIGHT (1<<4)
#define LCD_STATE_RIGHT (1<<5)
static volatile uint8_t LCD_state=LCD_STATE_UP;

static uint8_t LCD_off_timer=0xFF;

void printLCD_Clock1(void);
void printLCD_Clock2(void);
void printLCD_DoW(void);
void printLCD_date(void);
void printLCD_temp(void);
void printLCD_ptn(void);
void TransmitByteSPI(uint8_t data);
void StartSPI(void);
void sendAll(uint8_t reg, uint8_t data);

void SPI_MasterInit(void)
{
	/* Set MOSI and SCK output, all others input */
	DDRB |= (1<<PB3)|(1<<PB5)|(1<<PB2);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPIE)|(1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void Matrix_Init(void)
{
	SPI_buf_Head = 0;
	SPI_buf_Tail = 0;
	SPI_buf_Counter = 0;
	SPI_flag = 0;
	SPI_MasterInit();
	printLCD_Clock1();
	printLCD_Clock2();
	printLCD_DoW();
	printLCD_date();
	printLCD_temp();
	printLCD_ptn();
	sendAll(max7219_reg_scanLimit, 0x07);
	sendAll(max7219_reg_decodeMode, 0x00);  // using an led matrix (not digits)
	sendAll(max7219_reg_shutdown, 0x01);    // not in shutdown mode
	sendAll(max7219_reg_displayTest, 0x00); // no display test
	sendAll(max7219_reg_intensity, 0x0f & 0x01);    // the first 0x0f is the value you can set range: 0x00 to 0x0f

	TCCR0A |= 1 << WGM01;
	OCR0A = 0xFF;
	TCCR0B |= (1 << CS02)|(1 << CS00);
	TIMSK0 |= 1 << OCIE0A;
}

void sendAll(uint8_t reg, uint8_t data)
{
	for(uint8_t i=0; i<max7219InUse; i++)
	{
		TransmitByteSPI(reg);
		TransmitByteSPI(data);
	}
	StartSPI();
}

void StartSPI(void)
{
	if(!SPI_flag)
	{
		SPDR = 0;
		SPI_flag = 1;
	}
}

void TransmitByteSPI(uint8_t data)
{
	uint8_t tmphead;

	tmphead = SPI_buf_Head + 1;
	while(tmphead == SPI_buf_Tail);
	SPI_buf[tmphead] = data;
	SPI_buf_Head = tmphead;
}

void printLCD_Clock1(void)
{
	uint8_t h = get_hour_bcd();
	uint8_t h_h = h >> 4;
	h &= 0x0F;
	uint8_t m = get_minute_bcd();
	uint8_t m_h = m >> 4;
	m &= 0x0F;

	for (uint8_t i=0; i<30; i+=2)
	{
		LCD1_buf[i>>1][0] = tdDigital15table(h_h, i);
		LCD1_buf[i>>1][1] = tdDigital15table(h_h, i+1) | (tdDigital15table(h, i) >> 4);
		LCD1_buf[i>>1][2] = (tdDigital15table(h, i) << 4) | (tdDigital15table(h, i+1) >> 4);
		LCD1_buf[i>>1][3] = tdDigital15table(m_h, i) >> 1;
		LCD1_buf[i>>1][4] = (tdDigital15table(m_h, i) << 7) | (tdDigital15table(m_h, i+1) >> 1) | (tdDigital15table(m, i) >> 5);
		LCD1_buf[i>>1][5] = (tdDigital15table(m, i) << 3) | (tdDigital15table(m, i+1) >> 5);
	}
}

void printLCD_Clock2(void)
{
	uint8_t h = get_hour_bcd();
	uint8_t h_h = h >> 4;
	h &= 0x0F;
	uint8_t m = get_minute_bcd();
	uint8_t m_h = m >> 4;
	m &= 0x0F;
	uint8_t s = get_second_bcd();
	uint8_t s_h = s >> 4;
	s &= 0x0F;

	if(h_h == 0)
	{
		for (uint8_t i=0; i<7; i++)
		{
			LCD2_buf[i][0] = 0;
		}
	}
	else
	{
		for (uint8_t i=0; i<7; i++)
		{
			LCD2_buf[i][0] = dDigital7table(h_h, i) >> 1;
		}
	}

	for (uint8_t i=0; i<7; i++)
	{
		LCD2_buf[i][1] = dDigital7table(h, i);
		LCD2_buf[i][2] = dDigital7table(m_h, i) >> 2;
		LCD2_buf[i][3] = dDigital7table(m, i) >> 1;
		LCD2_buf[i][4] = dDigital7table(s_h, i) >> 3;
		LCD2_buf[i][5] = dDigital7table(s, i) >> 2;
	}

	if(s & 0x01)
	{
		LCD2_buf[1][1] |= 0x01;
		LCD2_buf[5][1] |= 0x01;
		LCD2_buf[1][4] |= 0x80;
		LCD2_buf[5][4] |= 0x80;
	}
}

void printLCD_DoW(void)
{
	uint8_t dow = get_dow();
	uint8_t shift=get_dow_shift(dow);
	uint8_t col, col_shift, c, c_len;

	for(uint8_t i=0; i<8; i++)
		for(uint8_t j=0; j<6; j++)
			LCD3_buf[i][j] = 0;

	for(uint8_t i=0; i<get_dow_len(dow); i++)
	{
		c = get_dow_str(dow, i);
		c_len = cyr7width(c);
		col = shift >> 3;
		col_shift = shift & 0x07;
		if(col_shift)
		{
			for (uint8_t j=0; j<7; j++)
			{
				LCD3_buf[j][col] |= cyr7table(c, j) >> col_shift;
			}
			if((col_shift + c_len) > 7)
			{
				col++;
				col_shift = 8 - col_shift;
				for (uint8_t j=0; j<7; j++)
				{
					LCD3_buf[j][col] = cyr7table(c, j) << col_shift;
				}
			}
		}
		else
		{
			for (uint8_t j=0; j<7; j++)
			{
				LCD3_buf[j][col] = cyr7table(c, j);
			}
		}
		shift += c_len + 1;
	}
}

void printLCD_date(void)
{
	uint8_t date = get_date_bcd();
	uint8_t date_h = date >> 4;
	date &= 0x0F;
	uint8_t mon = get_month() - 1;
    uint8_t width = _months_width(mon) + 6;
    if (date_h > 0) {
        width += 6;
    }
	uint8_t shift = 64 + (48 - width)/2; //get_mon_shift(mon) + 64;
	uint8_t col, col_shift, c, c_len;

	for(uint8_t i=0; i<8; i++)
		for(uint8_t j=8; j<14; j++)
			LCD3_buf[i][j] = 0;

	if(date_h)
	{
		col = shift >> 3;
		col_shift = shift & 0x07;
		for (uint8_t j=0; j<7; j++)
		{
			LCD3_buf[j][col] = dDigital7table(date_h, j) >> col_shift;
		}
		if((col_shift + 5) > 7)
		{
			col++;
			col_shift = 8 - col_shift;
			for (uint8_t j=0; j<7; j++)
			{
				LCD3_buf[j][col] = dDigital7table(date_h, j) << col_shift;
			}
		}
		shift += 5 + 1;
	}
	else
	{
		shift += 3;
	}

	col = shift >> 3;
	col_shift = shift & 0x07;
	if(col_shift)
	{
		for (uint8_t j=0; j<7; j++)
		{
			LCD3_buf[j][col] |= dDigital7table(date, j) >> col_shift;
		}
		if((col_shift + 5) > 7)
		{
			col++;
			col_shift = 8 - col_shift;
			for (uint8_t j=0; j<7; j++)
			{
				LCD3_buf[j][col] = dDigital7table(date, j) << col_shift;
			}
		}
	}
	else
	{
		for (uint8_t j=0; j<7; j++)
		{
			LCD3_buf[j][col] = dDigital7table(date, j);
		}
	}
	shift += 5 + 2;

    col = shift / 8;
    col_shift = shift & 0x07;
    for (uint8_t i=0; i<4; i++) {
        for (uint8_t j=0; j<5; j++) {
            uint8_t tmp = _months_table(mon, j, i);
            if (col_shift) {
				LCD3_buf[j+2][col] |= tmp >> col_shift;
                LCD3_buf[j+2][col+1] = tmp << 8 - col_shift;
			} else {
				LCD3_buf[j+2][col] = tmp;
			}
        }
        col++;
    }
}

void printLCD_temp(void)
{
	int8_t temp = get_temperature();
	int8_t temp_h=temp / 10;
	temp %= 10;

	for (uint8_t j=0; j<7; j++)
	{
		LCD3_buf[j][18] = dDigital7table(temp_h, j) | (dDigital7table(temp, j) >> 6);
	}

	for (uint8_t j=0; j<7; j++)
	{
		LCD3_buf[j][19] = (dDigital7table(temp, j) << 2);
	}

	LCD3_buf[0][19] |= 0x06;
	LCD3_buf[1][19] |= 0x09;
	LCD3_buf[2][19] |= 0x09;
	LCD3_buf[3][19] |= 0x06;
}

void printLCD_Illumination(void)
{
	uint16_t illumination = ADC;

	if(illumination > 999)
	{
		for (uint8_t j=0; j<7; j++)
		{
			LCD3_buf[j][25] = dDigital7table(1, j);
		}
		illumination -= 1000;
	}
	else
	{
		for (uint8_t j=0; j<7; j++)
		{
			LCD3_buf[j][25] = 0;
		}
	}
	uint8_t t = illumination / 100;
	for (uint8_t j=0; j<7; j++)
	{
		LCD3_buf[j][25] |= (dDigital7table(t, j) >> 6);
		LCD3_buf[j][26] = (dDigital7table(t, j) << 2);
	}
	illumination -= (uint16_t)t * 100;
	t = (uint8_t)illumination / 10;
	for (uint8_t j=0; j<7; j++)
	{
		LCD3_buf[j][26] |= (dDigital7table(t, j) >> 5);
		LCD3_buf[j][27] = (dDigital7table(t, j) << 3);
	}
	t = (uint8_t)illumination - t * 10;
	for (uint8_t j=0; j<7; j++)
	{
		LCD3_buf[j][27] |= (dDigital7table(t, j) >> 3);
	}
}

void printLCD_ptn(void)
{
	static uint8_t *str1 = "     ";
	static uint8_t *str2 = "     ";
	uint8_t shift=10;
	uint8_t col, col_shift, c, c_len;

	for (uint8_t i=0; i<5; i++)
	{
		c = str1[i];
		c_len = cyr7width(c);
		col = shift >> 3;
		col_shift = shift & 0x07;
		if(col_shift)
		{
			for (uint8_t j=0; j<7; j++)
			{
				LCD1_buf[j][col] |= cyr7table(c, j) >> col_shift;
			}
			if((col_shift + c_len) > 7)
			{
				col++;
				col_shift = 8 - col_shift;
				for (uint8_t j=0; j<7; j++)
				{
					LCD1_buf[j][col] = cyr7table(c, j) << col_shift;
				}
			}
		}
		else
		{
			for (uint8_t j=0; j<7; j++)
			{
				LCD1_buf[j][col] = cyr7table(c, j);
			}
		}
		shift += c_len + 2;
	}

	shift = 8;
	for (uint8_t i=0; i<5; i++)
	{
		c = str2[i];
		c_len = cyr7width(c);
		col = shift >> 3;
		col_shift = shift & 0x07;
		if(col_shift)
		{
			for (uint8_t j=0; j<7; j++)
			{
				LCD1_buf[j+8][col] |= cyr7table(c, j) >> col_shift;
			}
			if((col_shift + c_len) > 7)
			{
				col++;
				col_shift = 8 - col_shift;
				for (uint8_t j=0; j<7; j++)
				{
					LCD1_buf[j+8][col] = cyr7table(c, j) << col_shift;
				}
			}
		}
		else
		{
			for (uint8_t j=0; j<7; j++)
			{
				LCD1_buf[j+8][col] = cyr7table(c, j);
			}
		}
		shift += c_len + 2;
	}
}

void ClearLCDBuffer(void)
{
	for(uint8_t i=0; i<24; i++)
		for(uint8_t j=0; j<6; j++)
			LCD_buf[i][j] = 0;

	for(uint8_t i=24; i<32; i++)
		for(uint8_t j=0; j<32; j++)
			LCD_buf[i][j] = 0;
}

void LoadLCDBuffer(void)
{
	uint8_t tmp, col, row;
	uint8_t ColShift = LCD_buf_Col_Shift & 7;
	uint8_t ColMatrixShift = LCD_buf_Col_Shift >> 3;

	for(uint8_t r=0; r<max7219Rows; r++)
	{
		for(uint8_t m=0; m<max7219InUse; m++)
		{
			TransmitByteSPI(r + 1);

			if(m < max7219PerRow)
			{
				row = 15 - r + LCD_buf_Row_Shift;
				if(row < 24)
					tmp = LCD_buf[row][m];
				else
				{
					col = (m + ColMatrixShift) & 31;
					tmp = LCD_buf[row][col];
					if(ColShift)
					{
						tmp <<= ColShift;
						col++;
						col &= 31;
						tmp |= LCD_buf[row][col] >> (8 - ColShift);
					}
				}
				TransmitByteSPI(reverse(tmp));
			}
			else
			{
				row = r + LCD_buf_Row_Shift;
				col = 11 - m;
				if(row < 24)
					tmp = LCD_buf[row][col];
				else
				{
					col += ColMatrixShift;
					tmp = LCD_buf[row][col];
					if (ColShift)
					{
						tmp <<= ColShift;
						col++;
						tmp |= LCD_buf[row][col] >> (8 - ColShift);
					}
				}
				TransmitByteSPI(tmp);
			}
		}
	}
	StartSPI();
}

inline void max7219_cs(void)
{
	PORTB &= ~(1<<PB2);
	PORTB |= 1<<PB2;
}

SIGNAL(SPI_STC_vect)
{
	uint8_t tmptail;

	if(SPI_buf_Counter == max7219InUse*2)
	{
		SPI_buf_Counter = 0;
		max7219_cs();
	}
	if(SPI_buf_Head != SPI_buf_Tail)	//If Transmit Buffer Not Free, Transmit Byte
	{
		SPI_flag = 1;
		tmptail = SPI_buf_Tail + 1;
		SPI_buf_Tail = tmptail;
		SPI_buf_Counter++;
		SPDR = SPI_buf[tmptail];
	}
	else	//If Transmit Buffer Free
	{
		SPI_flag = 0;
		SPI_buf_Counter = 0;

	}
}

static volatile uint8_t scroll_counter=0;

SIGNAL(TIMER0_COMPA_vect)
{
	scroll_counter++;
	if((scroll_counter & 0x07) == 0)
	{
		switch(LCD_state)
		{
			case LCD_STATE_GOTO_UP:
				if(--LCD_buf_Row_Shift == 0)
					LCD_state = LCD_STATE_UP;
				LCD_update = 1;
				break;

			case LCD_STATE_GOTO_DOWN:
				if(++LCD_buf_Row_Shift == 16)
					LCD_state = LCD_STATE_DOWN;
				LCD_update = 1;
				break;

			case LCD_STATE_GOTO_RIGHT:
				if((++LCD_buf_Col_Shift & 0x3F) == 0)
					LCD_state = LCD_STATE_RIGHT;
				LCD_update = 1;
				break;
		}
	}
}

void MatrixUpdateHandler(void)
{
	if(LCD_update)
	{
		LCD_update = 0;
		LoadLCDBuffer();
	}
}

static uint8_t LCD_update_counter=0;

void TimeUpdateHandler(void)
{
	if(PIND & (1 << PD3))
	{
		if(LCD_off_timer == 0)
		{
			printLCD_Clock1();
			LCD_state = LCD_STATE_GOTO_UP;
			LCD_off_timer = 2;
		}
		else if(LCD_off_timer != 0xFF)
		{
			LCD_off_timer++;
		}
	}
	else if(LCD_off_timer)
	{
		LCD_off_timer--;
		if(LCD_off_timer == 0)
		{
			ClearLCDBuffer();
			LCD_buf_Row_Shift = 16;
			LCD_buf_Col_Shift = 0;
			LCD_state = LCD_STATE_DOWN;
			LCD_update = 1;
		}
	}

	if(LCD_off_timer)
	{
		if(LCD_state & (LCD_STATE_GOTO_UP | LCD_STATE_UP | LCD_STATE_GOTO_DOWN))
		{
			if(get_second_bcd() == 0)
			{
				printLCD_Clock1();
			}
		}

		if(LCD_state != LCD_STATE_GOTO_UP)
		{
			printLCD_Clock2();
			printLCD_Illumination();
			if(get_second_bcd() == 0)
			{
				if((get_hour() == 0) && (get_minute_bcd() == 0))
				{
					printLCD_DoW();
					printLCD_date();
				}
			}
			else if(get_second_bcd() == 0x30)
			{
				printLCD_temp();
			}
		}
		LCD_update = 1;

		switch(LCD_state)
		{
			case LCD_STATE_RIGHT:
				if(LCD_update_counter++ == 5)
				{
					LCD_state = LCD_STATE_GOTO_RIGHT;
					LCD_update_counter = 0;
					LCD_buf_Col_Shift++;
				}
				break;
			case LCD_STATE_DOWN:
				if(LCD_update_counter++ == 5)
				{
					LCD_state = LCD_STATE_GOTO_RIGHT;
					LCD_update_counter = 0;
				}
				break;
			case LCD_STATE_UP:
				if(LCD_update_counter++ == 5)
				{
					printLCD_Clock2();
					printLCD_DoW();
					printLCD_date();
					printLCD_temp();
					LCD_state = LCD_STATE_GOTO_DOWN;
					LCD_update_counter = 0;
					LCD_buf_Col_Shift = 0;
				}
				break;
			case LCD_STATE_GOTO_UP:
			case LCD_STATE_GOTO_DOWN:
				break;
			case LCD_STATE_GOTO_RIGHT:
				break;
		}
	}
}
