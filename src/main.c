#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdlib.h>

extern uint8_t asmfunction(uint8_t);
void cfunction(uint8_t);
uint8_t variable;

int main(void)
{
    uint8_t value;
    uint16_t time;
    variable = 12;
    value = 0;
    DDRB = 0xFF;
    sei(); // enables interrupts globally
    for (;;)
    {
        PORTB = value;
        value++;
        for (time = 0; time < 4; time++)
        {
            value = value;
        }
        value = asmfunction(value);
        cfunction(value);
    }
    cli(); // disables interrupts
}
void cfunction(uint8_t passing)
{
    PORTB = passing;
}
ISR(SIG_OVERFLOW0) // C handler for timer0
//overflow interrupt
// remember that SIGNAL does NOT re-enable
// interrupts inside the handler
// If you need a re-entrant handler then
// use INTERRUPT instead
{
    PORTB = 10;
    TCNT0 = 50;
}