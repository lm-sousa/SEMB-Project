#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include "AVR_CS.h"


#define Hz_10 6250  // compare match register 16MHz/256/10Hz
#define Hz_2  31250 // compare match register 16MHz/256/2Hz
#define Hz_2k 31    // compare match register 16MHz/256/2kHz

typedef struct {
    void (*function_pointer)(void);     // Pointer to task function.
    uint16_t program_counter;   // Pointer to the next instruction to be ran of this task.
    volatile uint16_t stack_ptr;         // Pointer to the address of the task's 'private' stack in memory
    uint8_t frequency;          // Holds interval of ticks between activations.
    uint8_t _cnt_to_activation; // Counts to zero. Activate function on zero.
} Task_cenas;

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

#define TASK(name, code) \
 void name##_f(void); \
 Task_cenas name = { \
    .function_pointer = name##_f , \
    .program_counter = 0, \
    .stack_ptr = 0, \
    .frequency = 0, \
    ._cnt_to_activation = 0 \
 }; \
 void name##_f(void) { \
    while (true) { \
        code \
    } \
    return; \
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

uint16_t test = 0;
volatile uint16_t* volatile pxCurrentTCB = &test;

void vPortYieldFromTick( void ) {
    // This is a naked function so the context
    // is saved. 
    portSAVE_CONTEXT();

    PORTD ^= _BV(6);
    if ((*pxCurrentTCB)) {
        PORTD |= _BV(5);    // Enable
    } else {
        PORTD &= ~_BV(5);   // Disable
    }

    // Increment the tick count and check to see
    // if the new tick value has caused a delay
    // period to expire. This function call can
    // cause a task to become ready to run.
    vTaskIncrementTick();
    
    // See if a context switch is required.
    // Switch to the context of a task made ready
    // to run by vTaskIncrementTick() if it has a
    // priority higher than the interrupted task.
    vTaskSwitchContext();
    
    // Restore the context. If a context switch
    // has occurred this will restore the context of
    // the task being resumed.
    portRESTORE_CONTEXT();
    
    // Return from this naked function.
    asm volatile ( "ret" );
}

uint8_t cnt = 20;

void vTaskIncrementTick() {
    // TODO
    if (cnt) {
        cnt--;
    } else {
        cnt = 20;
    }
    
    return;
}

int t1_inited = 0;
int t2_inited = 0;

void vTaskSwitchContext() {

    if (t1_inited && !t1.stack_ptr) {
        t1.stack_ptr=(*pxCurrentTCB);
    } else if (t2_inited && !t2.stack_ptr) {
        t2.stack_ptr=(*pxCurrentTCB);
    }

    if(!t1_inited) {
        t1_inited = 1;
        interrupts();
        t1.function_pointer();
    }
    else if(!t2_inited) {
        t2_inited = 1;
        interrupts();
        t2.function_pointer();
    }
    else {
        // TODO
        if (!(*pxCurrentTCB)) {
            (*pxCurrentTCB) = t1.stack_ptr;
        }
        if (!cnt) {
            PORTD ^= _BV(7);
            if ((*pxCurrentTCB) == t1.stack_ptr) {
                (*pxCurrentTCB) = t2.stack_ptr;
            } else if ((*pxCurrentTCB) == t2.stack_ptr) {
                (*pxCurrentTCB) = t1.stack_ptr;
            }
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
    addTask(t1.function_pointer);
    while (true) {
        asm("nop");
    }
}
