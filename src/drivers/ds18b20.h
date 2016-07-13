#ifndef DS18B20_H
#define DS18B20_H

#define DS18B20_INVALID_TEMPERATURE 0x8000

typedef void (*ds18b20_cb)(uint16_t data);

uint8_t ds18b20_convert(void);
uint8_t ds18b20_read_temperature(ds18b20_cb callback);

#endif  /* DS18B20_H */
