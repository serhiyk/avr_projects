#include <avr/io.h>
#include <avr/interrupt.h>
#include "SPI.h"
#include "config.h"
#include "nrf24l01_registers.h"
#include "nrf24l01.h"

#if !defined(CONFIG_NRF_CHANNEL)
    #error CONFIG_NRF_CHANNEL is not defined
#elif (CONFIG_NRF_CHANNEL > 125)
    #error CONFIG_NRF_CHANNEL > 125
#endif

#if !defined(CONFIG_NRF_POWER)
    #error CONFIG_NRF_POWER is not defined
#elif (CONFIG_NRF_POWER > 3)
    #error CONFIG_NRF_POWER > 3
#endif

#if !defined(CONFIG_NRF_DATA_RATE)
    #error CONFIG_NRF_DATA_RATE is not defined
#elif (CONFIG_NRF_DATA_RATE > 3)
    #error CONFIG_NRF_DATA_RATE > 3
#endif

#if !defined(CONFIG_NRF_AUTO_RETRANSMIT_DELAY)
    #error CONFIG_NRF_AUTO_RETRANSMIT_DELAY is not defined
#elif (CONFIG_NRF_AUTO_RETRANSMIT_DELAY > 15)
    #error CONFIG_NRF_AUTO_RETRANSMIT_DELAY > 15
#endif

#if !defined(CONFIG_NRF_AUTO_RETRANSMIT_COUNT)
    #error CONFIG_NRF_AUTO_RETRANSMIT_COUNT is not defined
#elif (CONFIG_NRF_AUTO_RETRANSMIT_COUNT > 15)
    #error CONFIG_NRF_AUTO_RETRANSMIT_COUNT > 15
#endif

#if !defined(NRF_CSN_PORT)
    #error NRF_CSN_PORT is not defined
#endif

#if !defined(NRF_CSN_DDR)
    #error NRF_CSN_DDR is not defined
#endif

#if !defined(NRF_CSN_PIN)
    #error NRF_CSN_PIN is not defined
#endif

#if !defined(NRF_CE_PORT)
    #error NRF_CE_PORT is not defined
#endif

#if !defined(NRF_CE_DDR)
    #error NRF_CE_DDR is not defined
#endif

#if !defined(NRF_CE_PIN)
    #error NRF_CE_PIN is not defined
#endif

#if defined(CONFIG_NRF_ENABLE_RX)
    #define NRF_EN_RXADDR ((1<<ERX_P0)|(0<<ERX_P1)|(0<<ERX_P2)|(0<<ERX_P3)|(0<<ERX_P4)|(0<<ERX_P5))
#else
    #define NRF_EN_RXADDR ((0<<ERX_P0)|(0<<ERX_P1)|(0<<ERX_P2)|(0<<ERX_P3)|(0<<ERX_P4)|(0<<ERX_P5))
#endif

#if defined(CONFIG_NRF_ENABLE_AUTO_ACKNOWLEDGMENT)
    #define NRF_EN_AA ((1<<ENAA_P0)|(0<<ENAA_P1)|(0<<ENAA_P2)|(0<<ENAA_P3)|(0<<ENAA_P4)|(0<<ENAA_P5))
#else
    #define NRF_EN_AA ((0<<ENAA_P0)|(0<<ENAA_P1)|(0<<ENAA_P2)|(0<<ENAA_P3)|(0<<ENAA_P4)|(0<<ENAA_P5))
#endif

#if defined(CONFIG_NRF_ENABLE_RX)
    #define NRF_CONFIG_DEFAULT ((0<<MASK_RX_DR)|(0<<MASK_TX_DS)|(0<<MASK_MAX_RT)|(1<<EN_CRC)|(1<<CRCO)|(1<<PWR_UP))
#else
    #define NRF_CONFIG_DEFAULT ((1<<MASK_RX_DR)|(0<<MASK_TX_DS)|(0<<MASK_MAX_RT)|(1<<EN_CRC)|(1<<CRCO)|(1<<PWR_UP))
#endif

#define NRF_CONFIG_RX (NRF_CONFIG_DEFAULT|(1<<PRIM_RX))
#define NRF_CONFIG_TX (NRF_CONFIG_DEFAULT|(0<<PRIM_RX))

static uint8_t nrf_status_flags;
static uint8_t nrf_pending_flag;
static volatile uint8_t nrf_int_flag=0;
static uint8_t *nrf_tx_buf;
static uint8_t nrf_tx_len;

#if defined(CONFIG_NRF_ENABLE_RX)
#if !defined(CONFIG_NRF_RX_MAX_LEN)
    #error CONFIG_NRF_RX_MAX_LEN is not defined
#elif (CONFIG_NRF_RX_MAX_LEN > 32)
    #error CONFIG_NRF_RX_MAX_LEN > 32
#endif
#if !defined(CONFIG_NRF_RX_PIPE_NUMBER)
    #error CONFIG_NRF_RX_PIPE_NUMBER is not defined
#elif (CONFIG_NRF_RX_PIPE_NUMBER > 32)
    #error CONFIG_NRF_RX_PIPE_NUMBER > 32
#endif
typedef struct {
    uint8_t rx_len;
    nrf_cb callback;
} nrf_descriptor_t;
static nrf_descriptor_t nrf_descriptor[CONFIG_NRF_RX_PIPE_NUMBER];
static uint8_t nrf_rx_buf[CONFIG_NRF_RX_MAX_LEN];
#endif

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

void _nrf_set_rx(void)
{
    if (nrf_status_flags)
    {
        _nrf_ce_low();
        _nrf_write_config(CONFIG, NRF_CONFIG_RX);
        _nrf_ce_high();
        nrf_status_flags = 0;
    }
}

void _nrf_set_tx(void)
{
    if (nrf_status_flags)
    {
        return;
    }
    _nrf_write_config(CONFIG, NRF_CONFIG_TX);
    nrf_status_flags = 1;
}

uint8_t _nrf_get_status(void)
{
    uint8_t buf[]={NOP};
    _nrf_csn_low();
    spi_master_transfer_blocking(buf, buf, sizeof(buf));
    _nrf_csn_high();
    return buf[0];
}

void nrf24_set_tx_addr(uint8_t *addr, uint8_t addr_len)
{
    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    _nrf_write_register(RX_ADDR_P0, addr, addr_len);
    _nrf_write_register(TX_ADDR, addr, addr_len);
}

void nrf24l01_init(void)
{
    NRF_CE_DDR |= 1 << NRF_CE_PIN;
    _nrf_ce_low();
    NRF_CSN_DDR |= 1 << NRF_CSN_PIN;
    _nrf_csn_high();

    _nrf_write_config(RF_CH, CONFIG_NRF_CHANNEL);
    _nrf_write_config(RX_PW_P0, 0x00);
    _nrf_write_config(RX_PW_P1, 0x00);
    _nrf_write_config(RX_PW_P2, 0x00);
    _nrf_write_config(RX_PW_P3, 0x00);
    _nrf_write_config(RX_PW_P4, 0x00);
    _nrf_write_config(RX_PW_P5, 0x00);
    _nrf_write_config(RF_SETUP, (CONFIG_NRF_DATA_RATE << RF_DR) | (CONFIG_NRF_POWER << RF_PWR));
    _nrf_write_config(CONFIG, NRF_CONFIG_TX);

#if defined(CONFIG_NRF_ENABLE_RX)
    _nrf_write_config(EN_AA, NRF_EN_AA);
#endif

    _nrf_write_config(EN_RXADDR, NRF_EN_RXADDR);
    _nrf_write_config(SETUP_RETR, (CONFIG_NRF_AUTO_RETRANSMIT_DELAY << ARD) | (CONFIG_NRF_AUTO_RETRANSMIT_COUNT << ARC));
    _nrf_write_config(DYNPD,(0<<DPL_P0)|(0<<DPL_P1)|(0<<DPL_P2)|(0<<DPL_P3)|(0<<DPL_P4)|(0<<DPL_P5));
    _nrf_write_config(STATUS, (1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));

#if defined(CONFIG_NRF_ENABLE_RX)
    uint8_t buf[] = {FLUSH_RX};
    _nrf_csn_low();
    spi_master_send_blocking(buf, sizeof(buf));
    _nrf_csn_high();

    nrf_status_flags = 1;
    _nrf_set_rx();
#endif

    EICRA |= 1 << ISC11; // The falling edge of INT1 generates an interrupt request
    EIMSK |= 1 << INT1; // External Interrupt Request 1 Enable
}

uint8_t nrf24_register_cb(uint8_t pipe, uint8_t *addr, uint8_t addr_len, uint8_t rx_len, nrf_cb callback)
{
#if defined(CONFIG_NRF_ENABLE_RX)
    if (pipe >= CONFIG_NRF_RX_PIPE_NUMBER || !callback)
    {
        return 1;
    }
    uint8_t tmp;

    // Set length of incoming payload
    _nrf_write_config(RX_PW_P0 + pipe, rx_len);

#if defined(CONFIG_NRF_ENABLE_AUTO_ACKNOWLEDGMENT)
    // Enable auto acknowledgment
    _nrf_read_register(EN_AA, &tmp, 1);
    tmp |= 1 << pipe;
    _nrf_write_config(EN_AA, tmp);
#endif

    // Enable RX address
    _nrf_read_register(EN_RXADDR, &tmp, 1);
    tmp |= 1 << pipe;
    _nrf_write_config(EN_RXADDR, tmp);

    // Set RX address
    _nrf_write_register(RX_ADDR_P0 + pipe, addr, addr_len);

    nrf_descriptor[pipe].rx_len = rx_len;
    nrf_descriptor[pipe].callback = callback;
    return 0;
#else
    return 1;
#endif
}

void nrf24_send(uint8_t *tx_buf, uint8_t len)
{
    nrf_tx_buf = tx_buf;
    nrf_tx_len = len;
    nrf_pending_flag = 1;
}

void _nrf_send_data_cb(void)
{
    _nrf_csn_high();
    _nrf_ce_high();
    nrf_pending_flag = 0;
}

void _nrf_send_data(void)
{
    _nrf_ce_low();
    _nrf_set_tx();
    _nrf_csn_low();
    uint8_t buf[] = {W_TX_PAYLOAD};
    spi_master_send_blocking(buf, 1);
    spi_master_send(nrf_tx_buf, nrf_tx_len, _nrf_send_data_cb);
}

void _nrf_get_data(uint8_t pipe)
{
#if defined(CONFIG_NRF_ENABLE_RX)
    uint8_t buf[] = {R_RX_PAYLOAD};
    _nrf_csn_low();
    spi_master_send_blocking(buf, 1);
    spi_master_transfer_blocking(nrf_rx_buf, nrf_rx_buf, nrf_descriptor[pipe].rx_len);
    _nrf_csn_high();
    if (nrf_descriptor[pipe].callback)
    {
        nrf_descriptor[pipe].callback(nrf_rx_buf);
    }
#endif
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
        if (t & (1 << MAX_RT))
        {
            uart_send_byte('F');
            _nrf_csn_low();
            uint8_t buf[] = {FLUSH_TX};
            spi_master_send_blocking(buf, 1);
            _nrf_csn_high();
#if defined(CONFIG_NRF_ENABLE_RX)
            _nrf_set_rx();
#endif
        }
        if (t & (1 << TX_DS))
        {
#if defined(CONFIG_NRF_ENABLE_RX)
            _nrf_set_rx();
#endif
        }
#if defined(CONFIG_NRF_ENABLE_RX)
        if (t & (1 << RX_DR))
        {
            uint8_t pipe = (t >> 1) & 7;
            _nrf_get_data(pipe);
        }
#endif
        _nrf_write_config(STATUS,(1<<RX_DR)|(1<<TX_DS)|(1<<MAX_RT));
    }

    if (nrf_pending_flag)
    {
        if (!spi_ready())
        {
            return;
        }
        _nrf_send_data();
    }
}
