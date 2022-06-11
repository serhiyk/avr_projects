#ifndef CONFIG_H
#define CONFIG_H

/* Timer config */
#define TIMER_NUMBER 2

/* MAX7219 config */
#define MAX7219_NUMBER 4
#define MAX7219_CS_PORT PORTB
#define MAX7219_CS_DDR DDRB
#define MAX7219_CS_PIN PB2

/* 1wire config */
#define ONEWIRE_PORT PORTC
#define ONEWIRE_DDR DDRC
#define ONEWIRE_PORT_IN PINC
#define ONEWIRE_PIN PC5

/* ds18b20 */
// #define PARASITE_POWER

/* UART config */
#define BAUDRATE 57600

#endif  /* CONFIG_H */
