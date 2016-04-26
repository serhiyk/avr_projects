#include <avr/io.h>
#include <avr/interrupt.h>
#include "SPI.h"
#include "nrf24l01.h"

#define NRF_CSN_PORT PORTD
#define NRF_CSN_DDR DDRD
#define NRF_CSN_PIN PD5
#define NRF_CE_PORT PORTD
#define NRF_CE_DDR DDRD
#define NRF_CE_PIN PD4

#define NRF_CHANNEL 2
#define NRF_PAYLOAD_LENGTH 4

#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD       0x1C
#define FEATURE     0x1D

/* configuratio nregister */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0

/* enable auto acknowledgment */
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0

/* enable rx addresses */
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0

/* setup of address width */
#define AW          0 /* 2 bits */

/* setup of auto re-transmission */
#define ARD         4 /* 4 bits */
#define ARC         0 /* 4 bits */

/* RF setup register */
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      1 /* 2 bits */

/* general status register */
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1 /* 3 bits */
#define TX_FULL     0

/* transmit observe register */
#define PLOS_CNT    4 /* 4 bits */
#define ARC_CNT     0 /* 4 bits */

/* fifo status */
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

/* dynamic length */
#define DPL_P0      0
#define DPL_P1      1
#define DPL_P2      2
#define DPL_P3      3
#define DPL_P4      4
#define DPL_P5      5

/* Instruction Mnemonics */
#define R_REGISTER    0x00 /* last 4 bits will indicate reg. address */
#define W_REGISTER    0x20 /* last 4 bits will indicate reg. address */
#define REGISTER_MASK 0x1F
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define NOP           0xFF

static volatile uint8_t nrf_int_flag=0;

inline void _nrf_csn_low(void)
{
    NRF_CSN_PORT &= ~(1 << NRF_CSN_PIN);
}

inline void _nrf_csn_high(void)
{
    NRF_CSN_PORT |= 1 << NRF_CSN_PIN;
}

inline void _nrf_ce_low(void)
{
    NRF_CE_PORT &= ~(1 << NRF_CE_PIN);
}

inline void _nrf_ce_high(void)
{
    NRF_CE_PORT |= 1 << NRF_CE_PIN;
}

void _nrf_read_register(uint8_t reg, uint8_t* value, uint8_t len)
{
    uint8_t buf[1];
    buf[0] = R_REGISTER | reg;
    while(!spi_ready());
    _nrf_csn_low();
    spi_master_send(buf, sizeof(buf), 0);
    while(!spi_ready());
    // spi_transfer(R_REGISTER | (REGISTER_MASK & reg));
    spi_master_transfer(value, value, len);
    // nrf24_transferSync(value,value,len);
    while(!spi_ready());
    _nrf_csn_high();
}

void _nrf_write_register(uint8_t reg, uint8_t* value, uint8_t len)
{
    uint8_t buf[1];
    buf[0] = W_REGISTER | reg;
    while(!spi_ready());
    _nrf_csn_low();
    spi_master_send(buf, sizeof(buf), 0);
    while(!spi_ready());
    // spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
    spi_master_send(value, len);
    // nrf24_transmitSync(value,len);
    while(!spi_ready());
    _nrf_csn_high();
}

void _nrf_write_config(uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = reg | W_REGISTER;
    buf[1] = value;
    while(!spi_ready());
    _nrf_csn_low();
    spi_master_send(buf, sizeof(buf), 0);
    while(!spi_ready());
    _nrf_csn_high();
}

void _nrf_powerup_rx(void)
{
    uint8_t buf[] = {FLUSH_RX};
    _nrf_csn_low();
    spi_master_send(buf, sizeof(buf), 0);
    _nrf_csn_high();

    _nrf_write_config(STATUS,(1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));

    _nrf_ce_low();
    _nrf_write_config(CONFIG,((1<<EN_CRC)|(0<<CRCO))|((1<<PWR_UP)|(1<<PRIM_RX)));
    _nrf_ce_high();
}

void _nrf_powerup_tx(void)
{
    _nrf_write_config(STATUS,(1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));

    _nrf_write_config(CONFIG,nrf24_CONFIG|((1<<PWR_UP)|(0<<PRIM_RX)));
}

uint8_t _nrf_get_status(void)
{
    uint8_t buf={NOP};
    _nrf_csn_low();
    spi_master_transfer(buf, buf, 1, 0);
    _nrf_csn_high();
    return buf[0];
}

void nrf24l01_init(void)
{
    NRF_CE_DDR |= 1 << NRF_CE_PIN;
    _nrf_ce_low();
    NRF_CSN_DDR |= 1 << NRF_CSN_PIN;
    _nrf_csn_high();

    _nrf_write_config(RF_CH, NRF_CHANNEL);

    // Set length of incoming payload
    _nrf_write_config(RX_PW_P0, 0x00); // Auto-ACK pipe ...
    _nrf_write_config(RX_PW_P1, NRF_PAYLOAD_LENGTH); // Data payload pipe
    _nrf_write_config(RX_PW_P2, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P3, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P4, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P5, 0x00); // Pipe not used

    // 1 Mbps, TX gain: 0dbm
    _nrf_write_config(RF_SETUP, (0<<RF_DR)|((0x03)<<RF_PWR));

    // CRC enable, 1 byte CRC length
    _nrf_write_config(CONFIG,((1<<EN_CRC)|(0<<CRCO)));

    // Auto Acknowledgment
    _nrf_write_config(EN_AA,(1<<ENAA_P0)|(1<<ENAA_P1)|(0<<ENAA_P2)|(0<<ENAA_P3)|(0<<ENAA_P4)|(0<<ENAA_P5));

    // Enable RX addresses
    _nrf_write_config(EN_RXADDR,(1<<ERX_P0)|(1<<ERX_P1)|(0<<ERX_P2)|(0<<ERX_P3)|(0<<ERX_P4)|(0<<ERX_P5));

    // Auto retransmit delay: 1000 us and Up to 15 retransmit trials
    _nrf_write_config(SETUP_RETR,(0x04<<ARD)|(0x0F<<ARC));

    // Dynamic length configurations: No dynamic length
    _nrf_write_config(DYNPD,(0<<DPL_P0)|(0<<DPL_P1)|(0<<DPL_P2)|(0<<DPL_P3)|(0<<DPL_P4)|(0<<DPL_P5));

    // Start listening
    _nrf_powerup_rx();

    EICRA |= 1 << ISC11;    // The falling edge of INT1 generates an interrupt request
    EIMSK |= 1 << INT1; // External Interrupt Request 1 Enable
}

void nrf24_send(uint8_t* tx_buf, uint8_t len)
{
    uint8_t buf[1];
    /* Go to Standby-I first */
    _nrf_ce_low();

    /* Set to transmitter mode , Power up if needed */
    _nrf_powerup_tx();

    /* Do we really need to flush TX fifo each time ? */
    #if 1
        /* Pull down chip select */
        _nrf_csn_low();

        /* Write cmd to flush transmit FIFO */
        buf[0] = FLUSH_TX;
        while(!spi_ready());
        spi_master_send(buf, 1, 0);
        while(!spi_ready());

        /* Pull up chip select */
        _nrf_csn_high();
    #endif

    /* Pull down chip select */
    _nrf_csn_low();

    /* Write cmd to write payload */
    buf[0] = W_TX_PAYLOAD;
    spi_master_send(buf, 1, 0);

    /* Write payload */
    while(!spi_ready());
    spi_master_send(tx_buf, len, 0);
    while(!spi_ready());

    /* Pull up chip select */
    _nrf_csn_high();

    /* Start the transmission */
    _nrf_ce_high();
}

SIGNAL(INT1_vect)
{
    nrf_int_flag = 1;
}

void nrf24_handler(void)
{
    if (nrf_int_flag)
    {
        if (!spi_ready())
        {
            return;
        }
        nrf_int_flag = 0;
        _nrf_get_status();
    }
}
