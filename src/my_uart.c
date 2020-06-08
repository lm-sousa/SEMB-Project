#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include "my_uart.h"

#define clockCyclesToMicroseconds(a) (((a)*1000L) / (F_CPU / 1000L))
#define MICROSECONDS_PER_TIMER0_TICK     (clockCyclesToMicroseconds(64))
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

volatile unsigned long timer2_micros = 0;
volatile unsigned long timer2_millis = 0;
static unsigned char timer2_fract = 0;

volatile uint8_t count;


void uart_init() {
  // Upper and lower bytes of the calculated prescaler value for baud.
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

  // Configure data frame size to 8-bits.
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

  // Configure to enable transmitter.
  UCSR0B = _BV(TXEN0);
}

void uart_putchar(char c) {
  // Wait until the register to write to is free.
  loop_until_bit_is_set(UCSR0A, UDRE0);

  // Write the byte to the register.
  UDR0 = c;
}

void uart_putstr(char* data) {
  // Loop until end of string writing char by char.
  while (*data) {
    uart_putchar(*data++);
  }
}

ISR(TIMER2_OVF_vect) {
  // copy these to local variables so they can be stored in registers
  // (volatile variables must be read from memory on every access)
  unsigned long m = timer2_millis;
  unsigned char f = timer2_fract;

  m += MILLIS_INC;
  f += FRACT_INC;
  if (f >= FRACT_MAX) {
    f -= FRACT_MAX;
    m += 1;
  }

  timer2_fract = f;
  timer2_millis = m;
  timer2_micros += MICROSECONDS_PER_TIMER0_OVERFLOW;
}

unsigned long millis2() {
  unsigned long m;
  uint8_t oldSREG = SREG;

  // disable interrupts while we read timer0_millis or we might get an
  // inconsistent value (e.g. in the middle of a write to timer0_millis)
  cli();
  m = timer2_millis;
  SREG = oldSREG;

  return m;
}

unsigned long micros2() {
    //
    // variables to hold local copies of volatile data
    //
	unsigned long m;            // microsecond counter
    uint8_t t;                  // timer0 count
    uint8_t f;                  // timer0 interrupt flags
	//
    // disable interrupts while acquiring volatile data
    //
	uint8_t oldSREG = SREG;
	cli();
    //
    // once interrupts are disabled, acquire all of the volatile data as 
    // fast as possible to minimize overall system interrupt latency.
    // there are three pieces of data to get: 
    // 1) 32-bit microsecond counter, 2) timer0 count, 3) timer0 interrupt flags
    //
	m = timer2_micros;

#if defined(TCNT0)
	t = TCNT0;
#elif defined(TCNT0L)
	t = TCNT0L;
#else
	#error TIMER 0 not defined
#endif
    //
    // there's a very small chance an interrupt will occur exactly here,
    // after reading timer0 count but before reading the interrupt flags.
    // this situation is detectable because timer0 count will be 255.
    //
#ifdef TIFR0
	f = TIFR0;
#else
	f = TIFR;
#endif

	SREG = oldSREG;

    if ((f & _BV(TOV0)) && (t < 255)) 
    {
        // 
        // if there's an un-serviced interrupt, then increment the microsecond
        // counter as if it had been serviced -- unless the special case mentioned
        // above has occurred.
        //
        m += MICROSECONDS_PER_TIMER0_OVERFLOW;
    }
    //
    // this is quite a bit faster than the previous code for two reasons:
    // 1) there's no need to shift m left 8 bits before the addition
    // 2) there's no call to the 32-bit multiply routine -- that takes
    //    about 22 cycles plus the overhead of two nested call/return instructions.
    //
    return m + ((uint16_t)t * MICROSECONDS_PER_TIMER0_TICK);
}

void start_counting_time() {
  DDRB |= (1 << 0);
  TCNT0 = 0;
  count = 0;
  TCCR0B |= (0 << CS02) | (1 << CS01) | (1 << CS00);  // PRESCALER 64
  TIMSK0 = (1 << TOIE0);
  sei();
}
