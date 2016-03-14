#include <avr/io.h>
#include <avr/interrupt.h>
#include "TWI.h"

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

typedef enum {
    TWI_IDLE = 0,
    TWI_MASTER_TX = 1,
    TWI_MASTER_RX = 2
} twi_state_t;

typedef struct {
    uint8_t dev_addr;
    uint8_t tx_len;
    uint8_t rx_len;
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    uint8_t index;
    uint8_t state;
    twi_cb callback;
} twi_descriptor_t;

static volatile twi_descriptor_t twi_descriptor;

inline void twi_send_byte(uint8_t data)
{
    TWDR = data;    //Save Data To The TWDR
    TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT);   //Begin Send
}

inline void twi_send_start(void)
{
    TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWSTA);    //send start condition
}

inline void twi_send_stop(void)
{
    //Transmit Stop Condition
    //Leave With TWEA On For Slave Receiving
    TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWEA) | (1 << TWSTO);
}

SIGNAL(TWI_vect)
{
    uint8_t status = TWSR & TWSR_STATUS_MASK;   //Read Status Bits
    switch(status) {
        // Master General
        case TW_START:  //0x08: Sent Start Condition
            twi_send_byte(twi_descriptor.dev_addr);  //Send Device Address
            break;
        case TW_REP_START:  //0x10: Sent Repeated Start Condition
            twi_send_byte(twi_descriptor.dev_addr | 1);   //Send Device Address
            break;
        // Master Transmitter & Receiver status codes
        case TW_MT_SLA_ACK: //0x18: Slave Address Acknowledged
        case TW_MT_DATA_ACK:    //0x28: Data Acknowledged
            if(twi_descriptor.state == TWI_MASTER_TX)
            {
                if(twi_descriptor.index < twi_descriptor.tx_len)
                {
                    twi_send_byte(twi_descriptor.tx_buf[twi_descriptor.index++]);
                }
                else if(twi_descriptor.rx_len > 0)
                {
                    twi_send_start();
                    twi_descriptor.index = 0;
                    if (twi_descriptor.rx_len == 1)
                    {
                        TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT);   //Data Byte Will Be Received, Reply With NACK (Final Byte In Transfer)
                    }
                    twi_descriptor.state = TWI_MASTER_RX;
                }
                else
                {
                    twi_send_stop();
                    if (twi_descriptor.callback)
                    {
                        twi_descriptor.callback();
                    }
                    twi_descriptor.state = TWI_IDLE;
                }
            }
            break;
        case TW_MR_DATA_ACK:    //0x50: Data Received, ACK Reply Issued
            twi_descriptor.rx_buf[twi_descriptor.index++] = TWDR;
            if(twi_descriptor.index + 1 < twi_descriptor.rx_len)
            {
                TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWEA); //Data Byte Will Be Received, Reply With ACK
            }
            else
            {
                TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT);   //Data Byte Will Be Received, Reply With NACK (Final Byte In Transfer)
            }
            break;
        case TW_MR_DATA_NACK:   //0x58: Data Received, NACK Reply Issued
            twi_descriptor.rx_buf[twi_descriptor.index++] = TWDR;   //Store Final Received Data Byte
            twi_send_stop();
            if (twi_descriptor.callback)
            {
                twi_descriptor.callback();
            }
            twi_descriptor.state = TWI_IDLE;
            break;
        case TW_MR_SLA_NACK:    //0x48: Slave Address Not Acknowledged
        case TW_MT_SLA_NACK:    //0x20: Slave Address Not Acknowledged
        case TW_MT_DATA_NACK:   //0x30: Data Not Acknowledged
            twi_send_stop();  //Transmit Stop Condition, Enable SLA ACK
            twi_descriptor.state = TWI_IDLE;    //Set State
            break;
        case TW_MR_SLA_ACK: //0x40: Slave Address Acknowledged
            TWCR = (TWCR & TWCR_CMD_MASK) | (1 << TWINT) | (1 << TWEA); //Data Byte Will Be Received, Reply With ACK
    }
}

void twi_init(void)
{
    uint8_t bitrate_div;

    PORTC |= (1<<PC4) | (1<<PC5);   //Set Pull-up Resistors On TWI Bus Pins (SCL, SDA)

    //Set TWI Bitrate
    //SCL freq = F_CPU/(16+2*TWBR))
    //For Processors With Additional Bitrate Division (mega128)
    //SCL freq = F_CPU/(16+2*TWBR*4^TWPS)
    TWSR &= ~((1 << TWPS0) | (1 << TWPS1) );    //Set TWPS To Zero
    bitrate_div = ((F_CPU / 1000l) / 400);  //Set twi Bit Rate To 100KHz
    if(bitrate_div >= 16)
        bitrate_div = (bitrate_div - 16) / 2;
    TWBR = bitrate_div;
    TWCR |= (1 << TWIE) | (1 << TWEA) | (1 << TWEN); //Enable TWI Interrupt, Slave Address ACK, TWI
}

uint8_t twi_master_transfer(uint8_t dev_addr, uint8_t *tx_buf, uint8_t *rx_buf, uint8_t tx_len, uint8_t rx_len, twi_cb callback)
{
    if (twi_descriptor.state != TWI_IDLE)
    {
        return -1;
    }
    twi_descriptor.tx_buf = tx_buf;
    twi_descriptor.rx_buf = rx_buf;
    twi_descriptor.tx_len = tx_len;
    twi_descriptor.rx_len = rx_len;
    twi_descriptor.index = 0;
    twi_descriptor.callback = callback;
    if (tx_len > 0)
    {
        twi_descriptor.state = TWI_MASTER_TX;
    }
    else
    {
        twi_descriptor.state = TWI_MASTER_RX;
        dev_addr |= 1;
    }
    twi_descriptor.dev_addr = dev_addr;
    twi_send_start();
    return 0;
}

uint8_t twi_ready(void)
{
    return twi_descriptor.state == TWI_IDLE;
}
