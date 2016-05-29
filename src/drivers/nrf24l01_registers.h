#ifndef NRF24L01_REGISTERS_H
#define NRF24L01_REGISTERS_H

/* configuration register */
#define CONFIG      0x00
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0

/* enable auto acknowledgment */
#define EN_AA       0x01
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0

/* enable rx addresses */
#define EN_RXADDR   0x02
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0

/* setup of address widths */
#define SETUP_AW    0x03
#define AW          0 /* 2 bits */

/* setup of auto retransmission */
#define SETUP_RETR  0x04
#define ARD         4 /* 4 bits */
#define ARC         0 /* 4 bits */

/* rf channel */
#define RF_CH       0x05 /* 7 bits */

/* rf setup register */
#define RF_SETUP    0x06
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      1 /* 2 bits */
#define LNA_HCURR   0

/* status register */
#define STATUS      0x07
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1 /* 3 bits */
#define TX_FULL     0

/* transmit observe register */
#define OBSERVE_TX  0x08
#define PLOS_CNT    4 /* 4 bits */
#define ARC_CNT     0 /* 4 bits */

/* carrier detect */
#define CD          0x09

/* receive address data pipe 0 */
#define RX_ADDR_P0  0x0A

/* receive address data pipe 1 */
#define RX_ADDR_P1  0x0B

/* receive address data pipe 2 */
#define RX_ADDR_P2  0x0C

/* receive address data pipe 3 */
#define RX_ADDR_P3  0x0D

/* receive address data pipe 4 */
#define RX_ADDR_P4  0x0E

/* receive address data pipe 5 */
#define RX_ADDR_P5  0x0F

/* transmit address */
#define TX_ADDR     0x10

/* number of bytes in rx payload in data pipe 0 */
#define RX_PW_P0    0x11

/* number of bytes in rx payload in data pipe 1 */
#define RX_PW_P1    0x12

/* number of bytes in rx payload in data pipe 2 */
#define RX_PW_P2    0x13

/* number of bytes in rx payload in data pipe 3 */
#define RX_PW_P3    0x14

/* number of bytes in rx payload in data pipe 4 */
#define RX_PW_P4    0x15

/* number of bytes in rx payload in data pipe 5 */
#define RX_PW_P5    0x16

/* fifo status */
#define FIFO_STATUS 0x17
#define TX_REUSE    6
// #define TX_FULL     5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

/* enable dynamic payload length */
#define DYNPD       0x1C
#define DPL_P5      5
#define DPL_P4      4
#define DPL_P3      3
#define DPL_P2      2
#define DPL_P1      1
#define DPL_P0      0

/* feature register */
#define FEATURE     0x1D
#define EN_DPL      2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

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

#endif  /* NRF24L01_REGISTERS_H */
