#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdlib.h>

extern uint8_t asmfunction();
int cfunction(uint8_t, uint32_t);
uint8_t value = 0;

int main(void)
{
    PORTB = 0x02;
    asmfunction();
}

int cfunction(uint8_t a, uint32_t b) {
    return PORTB = a + b;
}