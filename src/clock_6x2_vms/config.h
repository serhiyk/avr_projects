#ifndef CONFIG_H
#define CONFIG_H

/* Timer config */
#define TIMER_NUMBER 4

/* MAX7219 config */
#define MAX7219_NUMBER 12
#define MAX7219_CS_PORT PORTB
#define MAX7219_CS_DDR DDRB
#define MAX7219_CS_PIN PB2

/* BMP180 config */
#define BMP180_TIMER_ID 3
#define BMP180_READ_PERIOD_S 20

/* 1wire config */
#define ONEWIRE_PORT PORTD
#define ONEWIRE_DDR DDRD
#define ONEWIRE_PORT_IN PIND
#define ONEWIRE_PIN PD2

/* ds18b20 */
#define PARASITE_POWER

/* UART config */
#define BAUDRATE 9600

#endif  /* CONFIG_H */
