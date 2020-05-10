#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include "AVR_CS.h"


#define Hz_10 6250  // compare match register 16MHz/256/10Hz
#define Hz_2  31250 // compare match register 16MHz/256/2Hz
#define Hz_2k 31    // compare match register 16MHz/256/2kHz

void hardwareInit(int comp){
    noInterrupts();  // disable all interrupts
    
    pinMode(PD3, OUTPUT);
    pinMode(PD4, OUTPUT);
    pinMode(PD5, OUTPUT);
    pinMode(PD6, OUTPUT);
    pinMode(PD7, OUTPUT);

    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    
    OCR1A = comp;
    TCCR1B |= (1 << WGM12);     // CTC mode
    TCCR1B |= (1 << CS12);      // 256 prescaler
    TIMSK1 |= (1 << OCIE1A);    // enable timer compare interrupt

    interrupts();  // enable all interrupts
}

#define TASK(name, code) void name(void) __attribute__ ((naked)); \
 void name(void) { \
    while (true) { \
        code \
    } \
    asm volatile ( "ret" ); \
 }

TASK(t1, {
    for(int i = 0; i < 10000; i++) {
        asm("nop");
    }
    digitalWrite(PD3, !digitalRead(PD3));
});

TASK(t2, {
    for(int i = 0; i < 10000; i++) {
        asm("nop");
    }
    digitalWrite(PD4, !digitalRead(PD4));
});

void addTask(void (*f)()) {
    // TODO
    return;
}

void vPortYieldFromTick( void ) __attribute__ ( ( naked ) );
void vTaskIncrementTick( void );
void vTaskSwitchContext( void );

volatile TCB_t * volatile pxCurrentTCB = NULL;

void vPortYieldFromTick( void ) {
    /* This is a naked function so the context
    is saved. */
    portSAVE_CONTEXT();

    /* Increment the tick count and check to see
    if the new tick value has caused a delay
    period to expire. This function call can
    cause a task to become ready to run. */
    vTaskIncrementTick();
    
    /* See if a context switch is required.
    Switch to the context of a task made ready
    to run by vTaskIncrementTick() if it has a
    priority higher than the interrupted task. */
    vTaskSwitchContext();
    
    /* Restore the context. If a context switch
    has occurred this will restore the context of
    the task being resumed. */
    digitalWrite(PD7, !digitalRead(PD7));
    portRESTORE_CONTEXT();
    
    /* Return from this naked function. */
    asm volatile ( "ret" );
}

uint8_t cnt = 20;

void vTaskIncrementTick() {
    // TODO
    if (cnt) {
        cnt--;
        digitalWrite(PD5, !digitalRead(PD5));
    } else {
        cnt = 20;
    }
    
    return;
}

void vTaskSwitchContext() {
    // TODO
    if (!pxCurrentTCB) {
        pxCurrentTCB = t1;
    }
    if (!cnt) {
        if (pxCurrentTCB == t1) {
            pxCurrentTCB = t2;
        } else if (pxCurrentTCB == t2) {
            pxCurrentTCB = t1;
        }
    }
    return;
}

ISR(TIMER1_COMPA_vect, ISR_NAKED) {
    /* Call the tick function. */
    vPortYieldFromTick();

    /* Return from the interrupt. If a context
    switch has occurred this will return to a
    different task. */
    asm volatile ( "reti" );
}

int main() {
    hardwareInit(Hz_2);
    addTask(t1);
    while (true) {
        digitalWrite(PD6, !digitalRead(PD6));
        for(int i = 0; i < 1000; i++) {
            asm("nop");
        }
    }
}
