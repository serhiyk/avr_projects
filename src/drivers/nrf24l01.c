#include <avr/io.h>
#include <avr/interrupt.h>
#include "SPI.h"
#include "nrf24l01_registers.h"
#include "nrf24l01.h"

#define NRF_CSN_PORT PORTD
#define NRF_CSN_DDR DDRD
#define NRF_CSN_PIN PD5
#define NRF_CE_PORT PORTD
#define NRF_CE_DDR DDRD
#define NRF_CE_PIN PD4

#define NRF_CHANNEL 2
#define NRF_PAYLOAD_LENGTH 4
#define NRF_ADDR_LEN 5

static volatile uint8_t nrf_int_flag=0;
static uint8_t nrf_buf[NRF_BUF_SIZE];

typedef struct {
    uint8_t rx_len;
    nrf_cb callback;
} nrf_descriptor_t;

static nrf_descriptor_t nrf_descriptor[NRF_RX_PIPE_NUMBER];

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
    while(!spi_ready());
    _nrf_csn_low();
    spi_master_send_blocking(&reg, 1);
    spi_master_transfer_blocking(value, value, len);
    _nrf_csn_high();
}

void _nrf_write_register(uint8_t reg, uint8_t *value, uint8_t len)
{
    while(!spi_ready());
    _nrf_csn_low();
    reg |= W_REGISTER;
    spi_master_send_blocking(&reg, 1);
    spi_master_send_blocking(value, len);
    _nrf_csn_high();
}

void _nrf_write_config(uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    while(!spi_ready());
    _nrf_csn_low();
    buf[0] = reg | W_REGISTER;
    buf[1] = value;
    spi_master_send_blocking(buf, sizeof(buf));
    _nrf_csn_high();
}

void _nrf_powerup_rx(void)
{
    uint8_t buf[] = {FLUSH_RX};
    while(!spi_ready());
    _nrf_csn_low();
    spi_master_send_blocking(buf, sizeof(buf));
    _nrf_csn_high();
    _nrf_write_config(STATUS, (1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));
    _nrf_ce_low();
    _nrf_write_config(CONFIG, (1<<EN_CRC)|(0<<CRCO)|(1<<PWR_UP)|(1<<PRIM_RX));
    _nrf_ce_high();
}

void _nrf_powerup_tx(void)
{
    _nrf_write_config(STATUS, (1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));
    _nrf_write_config(CONFIG, (1<<EN_CRC)|(0<<CRCO)|(1<<PWR_UP)|(0<<PRIM_RX));
}

uint8_t _nrf_get_status(void)
{
    uint8_t buf[]={NOP};
    _nrf_csn_low();
    spi_master_transfer_blocking(buf, buf, sizeof(buf));
    _nrf_csn_high();
    return buf[0];
}

void nrf24_set_tx_addr(uint8_t *addr)
{
    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    _nrf_write_register(RX_ADDR_P0, addr, NRF_ADDR_LEN);
    _nrf_write_register(TX_ADDR, addr, NRF_ADDR_LEN);
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
    _nrf_write_config(RX_PW_P1, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P2, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P3, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P4, 0x00); // Pipe not used
    _nrf_write_config(RX_PW_P5, 0x00); // Pipe not used

    // 1 Mbps, TX gain: 0dbm
    _nrf_write_config(RF_SETUP, (0<<RF_DR)|(0x03<<RF_PWR));

    // CRC enable, 1 byte CRC length
    _nrf_write_config(CONFIG, (1<<EN_CRC)|(0<<CRCO));

    // Auto Acknowledgment
    _nrf_write_config(EN_AA, (1<<ENAA_P0)|(0<<ENAA_P1)|(0<<ENAA_P2)|(0<<ENAA_P3)|(0<<ENAA_P4)|(0<<ENAA_P5));

    // Enable RX addresses
    _nrf_write_config(EN_RXADDR, (1<<ERX_P0)|(0<<ERX_P1)|(0<<ERX_P2)|(0<<ERX_P3)|(0<<ERX_P4)|(0<<ERX_P5));

    // Auto retransmit delay: 1000 us and Up to 15 retransmit trials
    _nrf_write_config(SETUP_RETR, (0x04<<ARD)|(0x0F<<ARC));

    // Dynamic length configurations: No dynamic length
    _nrf_write_config(DYNPD,(0<<DPL_P0)|(0<<DPL_P1)|(0<<DPL_P2)|(0<<DPL_P3)|(0<<DPL_P4)|(0<<DPL_P5));

    // Start listening
    _nrf_powerup_rx();

    EICRA |= 1 << ISC11; // The falling edge of INT1 generates an interrupt request
    EIMSK |= 1 << INT1; // External Interrupt Request 1 Enable
}

uint8_t nrf24_register_cb(uint8_t pipe, uint8_t *addr, uint8_t addr_len, uint8_t rx_len, nrf_cb callback)
{
    if (pipe == 0 || pipe > NRF_RX_PIPE_NUMBER || !callback)
    {
        return 1;
    }
    uint8_t tmp;

    // Set length of incoming payload
    _nrf_write_config(RX_PW_P0 + pipe, rx_len);

    // Enable auto acknowledgment
    _nrf_read_register(EN_AA, &tmp, 1);
    tmp |= 1 << pipe;
    _nrf_write_config(EN_AA, tmp);

    // Enable RX address
    _nrf_read_register(EN_RXADDR, &tmp, 1);
    tmp |= 1 << pipe;
    _nrf_write_config(EN_RXADDR, tmp);

    // Set RX address
    _nrf_write_register(RX_ADDR_P0 + pipe, addr, addr_len);

    pipe--;

    nrf_descriptor[pipe].rx_len = rx_len;
    nrf_descriptor[pipe].callback = callback;
    return 0;
}

void nrf24_send(uint8_t *tx_buf, uint8_t len)
{
    uint8_t buf[1];
    while(!spi_ready());
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
        // while(!spi_ready());
        spi_master_send_blocking(buf, 1);

        /* Pull up chip select */
        _nrf_csn_high();
    #endif

    /* Pull down chip select */
    _nrf_csn_low();

    /* Write cmd to write payload */
    buf[0] = W_TX_PAYLOAD;
    spi_master_send_blocking(buf, 1);

    /* Write payload */
    spi_master_send_blocking(tx_buf, len);

    /* Pull up chip select */
    _nrf_csn_high();

    /* Start the transmission */
    _nrf_ce_high();
}

void _nrf_get_data(uint8_t pipe)
{
    uint8_t buf[] = {R_RX_PAYLOAD};
    _nrf_csn_low();
    spi_master_send_blocking(buf, 1);
    spi_master_transfer_blocking(nrf_buf, nrf_buf, nrf_descriptor[pipe].rx_len);
    _nrf_csn_high();
    if (nrf_descriptor[pipe].callback)
    {
        nrf_descriptor[pipe].callback(nrf_buf);
    }
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
        uint8_t t =_nrf_get_status();
        uart_send_hex(t);
        if (t & (1 << MAX_RT))
        {
            uart_send_byte('F');
            _nrf_csn_low();
            /* Write cmd to flush transmit FIFO */
            uint8_t buf[] = {FLUSH_TX};
            spi_master_send_blocking(buf, 1);
            _nrf_csn_high();
            _nrf_powerup_rx();
        }
        if (t & (1 << TX_DS))
        {
            uart_send_byte('T');
            _nrf_powerup_rx();
        }
        if (t & (1 << RX_DR))
        {
            uart_send_byte('R');
            uint8_t pipe = ((t>>1)&7)-1;
            _nrf_get_data(pipe);
        }
        // uart_send_hex(t);
        _nrf_write_config(STATUS,(1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));
    }
}
