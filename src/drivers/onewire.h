#ifndef ONEWIRE_H
#define ONEWIRE_H

typedef void (*onewire_cb)(uint8_t data);

void onewire_init(void);
void onewire_master_pull_down(onewire_cb callback);
void onewire_master_pull_up(onewire_cb callback);
void onewire_master_write(uint8_t data, onewire_cb callback);
void onewire_master_read(onewire_cb callback);

#endif  /* ONEWIRE_H */
