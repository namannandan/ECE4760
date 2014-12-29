nclude <util/delay.h>

#include "avr_delay.h"

#define delay 1

//delay functions
void delayms(uint16_t n)
{
    for(;n>0;n--)
    {
        _delay_ms(delay);
    }

    return;
}

void delayus(uint16_t n)
{
    for(;n>0;n--)
    {
        _delay_us(delay);
    }

    return;
}

