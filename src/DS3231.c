#include <avr/io.h>
#include <avr/interrupt.h>
#include "UART.h"
#include "DS3231.h"

// TWI state and address variables
#define TW_START 0x08
#define TW_REP_START 0x10
#define TW_MT_SLA_ACK 0x18
#define TW_MT_SLA_NACK 0x20
#define TW_MT_DATA_ACK 0x28
#define TW_MT_DATA_NACK 0x30
#define TW_MR_SLA_ACK 0x40
#define TW_MR_SLA_NACK 0x48
#define TW_MR_DATA_ACK 0x50
#define TW_MR_DATA_NACK 0x58

#define TWCR_CMD_MASK 0x0F
#define TWSR_STATUS_MASK 0xF8

#define CLOCK_ADDRESS	0xD0
#define DEVICE_WRITE    CLOCK_ADDRESS //Default TWI address - write
#define DEVICE_READ     (CLOCK_ADDRESS | 1) //Default TWI address - read

typedef enum
{
	TWI_IDLE = 0,
	TWI_BUSY = 1,
	TWI_MASTER_TX = 2,
	TWI_MASTER_RX = 3,
	TWI_MASTER_TX_F = 4
} eTWIstateType;

static volatile eTWIstateType TWIstate;
static volatile uint8_t TWIbuf[19];
static volatile uint8_t TWIbufIndex;
static volatile uint8_t TWIbufLength;
static volatile uint8_t TWIaddr;

volatile uint8_t RTC_int_flag;

inline void twiSendByte(uint8_t data)
{
	TWDR = data;	//Save Data To The TWDR
	TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT);	//Begin Send
}

inline void twiSendStart(void)
{
	TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWSTA);	//send start condition
}

inline void twiSendStop(void)
{
	//Transmit Stop Condition
	//Leave With TWEA On For Slave Receiving
	TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWEA) | (1 << TWSTO);
}

void ClearAlarm(void)
{
	while(TWIstate);
	TWIstate = TWI_MASTER_TX;
	
	TWIbuf[15] = 0x00;
	
	TWIaddr = 15;
	TWIbufIndex = 15;
	TWIbufLength = 16;
	twiSendStart();	//Send Start Condition
}

uint8_t getSecond(void)
{
	return bcdToDec(TWIbuf[0]);
}

uint8_t getSecondBCD(void)
{
	return TWIbuf[0];
}

uint8_t getMinute(void)
{
	return bcdToDec(TWIbuf[1]);
}

uint8_t getMinuteBCD(void)
{
	return TWIbuf[1];
}

uint8_t getHour(void)
{
	uint8_t temp_buffer;
	uint8_t hour;
	temp_buffer = TWIbuf[2];
	if(temp_buffer & 0b01000000)
		hour = bcdToDec(temp_buffer & 0b00011111);
	else
		hour = bcdToDec(temp_buffer & 0b00111111);
	return hour;
}

uint8_t getHourBCD(void)
{
	return TWIbuf[2];
}

uint8_t getDoW(void)
{
	return bcdToDec(TWIbuf[3]);
}

uint8_t getDate(void)
{
	return bcdToDec(TWIbuf[4]);
}

uint8_t getDateBCD(void)
{
	return TWIbuf[4];
}

uint8_t getMonth(void)
{
	return (bcdToDec(TWIbuf[5] & 0b01111111)) ;
}

uint8_t getYear(void)
{
	return bcdToDec(TWIbuf[6]);
}

void setTime(uint8_t Second, uint8_t Minute, uint8_t Hour, uint8_t DoW, uint8_t Date, uint8_t Month, uint8_t Year)
{
	while(TWIstate);
	TWIstate = TWI_MASTER_TX;
	
	TWIbuf[0] = decToBcd(Second);
	TWIbuf[1] = decToBcd(Minute);
	TWIbuf[2] = decToBcd(Hour);// & 0b10111111;
	TWIbuf[3] = decToBcd(DoW);
	TWIbuf[4] = decToBcd(Date);
	TWIbuf[5] = decToBcd(Month);
	TWIbuf[6] = decToBcd(Year);
	TWIbuf[7] = 0x80;
	TWIbuf[8] = 0x80;
	TWIbuf[9] = 0x80;
	TWIbuf[10] = 0x80;
	TWIbuf[11] = 0x80;
	TWIbuf[12] = 0x80;
	TWIbuf[13] = 0x80;
	TWIbuf[14] = 0x05; // Alarm 1 Interrupt Enable
	TWIbuf[15] = 0x00;
	
	TWIaddr = 0;
	TWIbufIndex = 0;
	TWIbufLength = 16;
	twiSendStart();	//Send Start Condition
}

int8_t getTemperature(void)
{
	return TWIbuf[0x11];	// Here's the MSB
}

uint8_t decToBcd(uint8_t val)
{
// Convert normal decimal numbers to binary coded decimal
	return ((val/10*16) + (val%10));
}

uint8_t bcdToDec(uint8_t val)
{
// Convert binary coded decimal to normal decimal numbers
	return ((val/16*10) + (val%16));
}

void TWI_Init(void)
{
	uint8_t bitrate_div;
	
	PORTC |= (1<<PC4) | (1<<PC5);	//Set Pull-up Resistors On TWI Bus Pins (SCL, SDA)
	
	//Set TWI Bitrate
	//SCL freq = F_CPU/(16+2*TWBR))
	//For Processors With Additional Bitrate Division (mega128)
	//SCL freq = F_CPU/(16+2*TWBR*4^TWPS)
	TWSR &= ~((1 << TWPS0) | (1 << TWPS1) );	//Set TWPS To Zero
	bitrate_div = ((F_CPU / 1000l) / 400);	//Set twi Bit Rate To 100KHz
	if(bitrate_div >= 16)
		bitrate_div = (bitrate_div - 16) / 2;
	TWBR = bitrate_div;
	
	TWIstate = TWI_IDLE;	//Set State
	
	TWCR |= (1 << TWIE) | (1 << TWEA) | (1 << TWEN); //Enable TWI Interrupt, Slave Address ACK, TWI
	
	EICRA |= 1 << ISC01;	// The falling edge of INT0 generates an interrupt request
	EIMSK |= 1 << INT0;	// External Interrupt Request 0 Enable
}

SIGNAL(TWI_vect)
{
	uint8_t status = TWSR & TWSR_STATUS_MASK;	//Read Status Bits
	//TransmitByte(status);
	switch(status)
	{
		// Master General
		case TW_START:	//0x08: Sent Start Condition
			twiSendByte(DEVICE_WRITE);	//Send Device Address
			break;
		case TW_REP_START:	//0x10: Sent Repeated Start Condition
			twiSendByte(DEVICE_READ);	//Send Device Address
			break;
		// Master Transmitter & Receiver status codes
		case TW_MT_SLA_ACK:	//0x18: Slave Address Acknowledged
			twiSendByte(TWIaddr);
			break;
		case TW_MT_DATA_ACK:	//0x28: Data Acknowledged
			if(TWIstate == TWI_MASTER_RX)	//ReStart
			{
				twiSendStart();
			}
			else if(TWIbufIndex < TWIbufLength)
			{
				twiSendByte(TWIbuf[TWIbufIndex++]);
			}
			else
			{
				twiSendStop();
				TWIstate = TWI_IDLE;
			}
			break;
		case TW_MR_DATA_ACK:	//0x50: Data Received, ACK Reply Issued
			TWIbuf[TWIbufIndex++] = TWDR;
			if(TWIbufIndex < TWIbufLength)
			{
				TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWEA);	//Data Byte Will Be Received, Reply With ACK
			}
			else
			{
				TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT);	//Data Byte Will Be Received, Reply With NACK (Final Byte In Transfer)
			}
			break;
		case TW_MR_DATA_NACK:	//0x58: Data Received, NACK Reply Issued
			TWIbuf[TWIbufIndex++] = TWDR;	//Store Final Received Data Byte
			RTC_int_flag = 1;
			twiSendStop();
			TWIstate = TWI_IDLE;
			break;
		case TW_MR_SLA_NACK:	//0x48: Slave Address Not Acknowledged
		case TW_MT_SLA_NACK:	//0x20: Slave Address Not Acknowledged
		case TW_MT_DATA_NACK:	//0x30: Data Not Acknowledged
			twiSendStop();	//Transmit Stop Condition, Enable SLA ACK
			TWIstate = TWI_IDLE;	//Set State
			break;
		case TW_MR_SLA_ACK:	//0x40: Slave Address Acknowledged
			TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWEA);	//Data Byte Will Be Received, Reply With ACK
	}
}

void readRTC(void)
{
	while(TWIstate);
	TWIstate = TWI_MASTER_RX;
	TWIbufIndex = 0;
	TWIbufLength = 18;
	TWIaddr = 0;
	twiSendStart();
	while(TWIstate);
}

SIGNAL(INT0_vect)
{
	if(TWIstate == TWI_IDLE)
	{
		TWIstate = TWI_MASTER_RX;
		TWIbufIndex = 0;
		TWIbufLength = 18;
		TWIaddr = 0;
		twiSendStart();
	}
}
