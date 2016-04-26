#include <avr/io.h>
#include <avr/interrupt.h>
#include "display.h"
#include "timer.h"
#include "motion.h"

#define MOTION_TIMER_ID 1
#define MOTION_TIMER_PERIOD 10 // 1 second

void motion_handler(void);

void motion_init(void)
{
	timer_register(MOTION_TIMER_ID, MOTION_TIMER_PERIOD, motion_handler);
}

void motion_handler(void)
{
    static uint8_t off_timer=0;
    if(PIND & (1 << PD6))
    {
        if(off_timer == 0)
        {
            display_activate();
            off_timer = 2;
        }
        else if(off_timer != 0xFF)
        {
            off_timer++;
        }
    }
    else if(off_timer)
    {
        off_timer--;
        if(off_timer == 0)
        {
            display_deactivate();
        }
    }
}
