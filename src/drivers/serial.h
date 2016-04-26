#ifndef SERIAL_H
#define SERIAL_H

typedef void (*command_cb)(uint8_t *cmd, uint8_t len);

void serial_init(void);
uint8_t serial_register_command(uint8_t id, command_cb callback);
void serial_handler(void);

#endif  /* SERIAL_H */
