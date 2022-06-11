#ifndef MAX7219_H_
#define MAX7219_H_

#define MAX7219_ROWS 8

void max7219_init(void);
void max7219_update(void);
void max7219_handler(void);
void max7219_update_with_config(void);

#endif /* MAX7219_H_ */
