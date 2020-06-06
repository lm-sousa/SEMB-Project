#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include "AVR_CS.h"

enum status {TASK_RUNNING, TASK_WAITING, TASK_READY};

#define Hz_1    62500   // compare match register 16MHz/256/1Hz
#define Hz_2    31250   // compare match register 16MHz/256/2Hz
#define Hz_4    15625   // compare match register 16MHz/256/4Hz
#define Hz_5    12500   // compare match register 16MHz/256/5Hz
#define Hz_10   6250    // compare match register 16MHz/256/10Hz
#define Hz_20   3125    // compare match register 16MHz/256/20Hz
#define Hz_50   1250    // compare match register 16MHz/256/50Hz
#define Hz_100  625     // compare match register 16MHz/256/100Hz
#define Hz_500  125     // compare match register 16MHz/256/500Hz
#define Hz_1k   62      // compare match register 16MHz/256/1kHz
#define Hz_2k   31      // compare match register 16MHz/256/2kHz
#define Hz_2k5  25      // compare match register 16MHz/256/2.5kHz
#define Hz_5k   12      // compare match register 16MHz/256/5kHz
#define Hz_10k  6       // compare match register 16MHz/256/10kHz
#define Hz_12k5 5       // compare match register 16MHz/256/12.5kHz
#define Hz_62k5 1       // compare match register 16MHz/256/62.5kHz

#define TICK_FREQUENCY Hz_10
#define TASK_FREQUENCY(freq_in_Hz_ints) freq_in_Hz_ints/TICK_FREQUENCY

#define STACK_SIZE_DEFAULT 100

#define STATUS_LED  5
#define TICK_LED    6
#define CS_LED      7

#define MAX_TASKS   14

#define NUMBER_OF_MUTEXES 16

#if !NUMBER_OF_MUTEXES || NUMBER_OF_MUTEXES > 32
    #error NUMBER_OF_MUTEXES needs to be a value between 1 and 32.
#endif

#if NUMBER_OF_MUTEXES <= 8
    typedef uint8_t mutex_mask_t;
#elif NUMBER_OF_MUTEXES <= 16
    typedef uint16_t mutex_mask_t;
#elif NUMBER_OF_MUTEXES <= 32
    typedef uint32_t mutex_mask_t;
#endif

#define TASK_REQUESTED_MUTEXES_ARE_UNLOCKED (!(tasks[i]->mutex_mask & mutex_master))

typedef struct {
    volatile uint8_t*   stack_ptr;                  // Pointer to the address of the task's 'private' stack in memory
    void                (*function_pointer)(void);  // Pointer to task function.
    uint16_t            _cnt_to_activation;         // Counts to zero. Activate function on zero.
    const uint8_t       priority;                   // Priority for fixed-priority scheduling
    uint8_t             status;                     // Status for scheduling.
    const uint16_t      frequency;                  // Number of ticks between activations
    mutex_mask_t        mutex_mask;                 // bitlist of currently waiting mutexes for this task
} Task_cenas;

mutex_mask_t mutex_master = 0; // mutexes are inited to 0

Task_cenas* tasks[MAX_TASKS] = {0};
uint8_t task_count = 0;
int task = 0;
bool from_suspension = false;

#define self *(tasks[task])
#define yield() from_suspension = true; vPortYieldFromTick();
#define suspend() from_suspension = true; tasks[task]->status = TASK_WAITING; yield(); continue;

void vPortYieldFromTick( void ) __attribute__ ( ( naked ) );
void vTaskIncrementTick( void );
void vTaskSwitchContext( void );
uint8_t *pxPortInitialiseStack( uint8_t* pxTopOfStack, void (*pxCode)(), void *pvParameters );

bool trylock(uint8_t index){
    if(index >= NUMBER_OF_MUTEXES)
        return 0;

    if(mutex_master & _BV(index))
        return 0;
    
    mutex_master |= _BV(index);
    return 1;
}

void lock(uint8_t index){
    if(index >= NUMBER_OF_MUTEXES)
        return;

    while(mutex_master & _BV(index)) {
        tasks[task]->mutex_mask |= _BV(index);
        yield();
    }
    tasks[task]->mutex_mask &= ~(_BV(index));
    
    mutex_master |= _BV(index);
    return;
}

void unlock(uint8_t index){
    if(index >= NUMBER_OF_MUTEXES)
        return;
    
    mutex_master &= ~(_BV(index));
    return;
}

void hardwareInit(){
    noInterrupts();  // disable all interrupts
    
    DDRD |= _BV(2);
    DDRD |= _BV(3);
    DDRD |= _BV(4);
    DDRD |= _BV(STATUS_LED);
    DDRD |= _BV(TICK_LED);
    DDRD |= _BV(CS_LED);

    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    
    OCR1A = TICK_FREQUENCY;
    TCCR1B |= (1 << WGM12);     // CTC mode
    TCCR1B |= (1 << CS12);      // 256 prescaler
    TIMSK1 |= (1 << OCIE1A);    // enable timer compare interrupt

    interrupts();  // enable all interrupts
}

#define TASK5(name, pr, fr, stack_size, code) \
 void name##_f(void) __attribute__ ( ( OS_task ) ); \
 uint8_t name##_stack[stack_size]; \
 Task_cenas name = { \
    .function_pointer = name##_f , \
    .stack_ptr = 0, \
    ._cnt_to_activation = 0, \
    .priority = pr, \
    .status = TASK_READY, \
    .frequency = TASK_FREQUENCY(fr), \
    .mutex_mask = 0 \
 }; \
 void name##_f(void) { \
    while (true) { \
        code \
    } \
    return; \
 }

#define TASK4(name, pr, fr, code)   TASK5(name, pr, fr, STACK_SIZE_DEFAULT, code)
#define TASK3(name, fr, code)       TASK4(name, 254, fr, code)
#define TASK2(name, code)           TASK3(name, Hz_1, code)

#define GET_TASK_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME
#define TASK(...) GET_TASK_MACRO(__VA_ARGS__, TASK5, TASK4, TASK3, TASK2)(__VA_ARGS__)

#define addTask(task) addTaskTest(&task, task##_stack)
uint8_t addTaskTest(Task_cenas* task, uint8_t* stack) {
    task->stack_ptr = pxPortInitialiseStack(stack+STACK_SIZE_DEFAULT, task->function_pointer, 0);
    tasks[task_count] = task;
    task_count++;
    return task_count - 1;
}

uint16_t test = 0;
volatile void* volatile pxCurrentTCB = &test;

uint8_t *pxPortInitialiseStack( uint8_t* pxTopOfStack, void (*pxCode)(), void *pvParameters ) {
    uint16_t usAddress;
    /* Simulate how the stack would look after a call to vPortYield() generated by
    the compiler. */

    /* The start of the task code will be popped off the stack last, so place
    it on first. */
    usAddress = ( uint16_t ) pxCode;
    *pxTopOfStack = ( uint8_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    usAddress >>= 8;
    *pxTopOfStack = ( uint8_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    /* Next simulate the stack as if after a call to portSAVE_CONTEXT().
    portSAVE_CONTEXT places the flags on the stack immediately after r0
    to ensure the interrupts get disabled as soon as possible, and so ensuring
    the stack use is minimal should a context switch interrupt occur. */
    *pxTopOfStack = ( uint8_t ) 0x00;    /* R0 */
    pxTopOfStack--;
    *pxTopOfStack = ( (uint8_t) 0x80 );
    pxTopOfStack--;

    /* Now the remaining registers. The compiler expects R1 to be 0. */
    *pxTopOfStack = ( uint8_t ) 0x00;    /* R1 */

    /* Leave R2 - R23 untouched */
    pxTopOfStack -= 23;

    /* Place the parameter on the stack in the expected location. */
    usAddress = ( uint16_t ) pvParameters;
    *pxTopOfStack = ( uint8_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    usAddress >>= 8;
    *pxTopOfStack = ( uint8_t ) ( usAddress & ( uint16_t ) 0x00ff );

    /* Leave register R26 - R31 untouched */
    pxTopOfStack -= 7;

    return pxTopOfStack;
}

void vPortYieldFromTick( void ) {
    // This is a naked function so the context
    // must be saved manually. 
    portSAVE_CONTEXT();

    // Increment the tick count and check to see
    // if the new tick value has caused a delay
    // period to expire. This function call can
    // cause a task to become ready to run.
    if (!from_suspension)
        vTaskIncrementTick();
    from_suspension = false;
    
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

void vTaskIncrementTick() {
    
    for (uint8_t i = 0; i < task_count; i++) {
        if (tasks[i]) {
            if (0 == tasks[i]->_cnt_to_activation) {
                tasks[i]->status = TASK_READY;
                tasks[i]->_cnt_to_activation = tasks[i]->frequency;
            }
            
            if (0 != tasks[i]->_cnt_to_activation) {
                (tasks[i]->_cnt_to_activation)--;
            }
        }
    }
    
    return;
}

void vTaskSwitchContext() {
    PORTD ^= _BV(STATUS_LED);    // Pisca-pisca no ma no ma ei

    if(tasks[task]->status == TASK_RUNNING)
        tasks[task]->status = TASK_READY;

    PORTD &= ~(_BV(CS_LED)); // turn off iddle task LED

    // find the highest priority task which is ready (i.e., task->priority is lowest)

    uint8_t run_next_id = 0;
    uint8_t run_next_pr = 255;
    for(uint8_t i = 0; i < task_count; i++){
        if(tasks[i] && tasks[i]->priority <= run_next_pr && tasks[i]->status == TASK_READY && TASK_REQUESTED_MUTEXES_ARE_UNLOCKED) {
            run_next_id = i;
            run_next_pr = tasks[i]->priority;
        }
    }

    task = run_next_id;
    tasks[task]->status = TASK_RUNNING;
    pxCurrentTCB = &tasks[task]->stack_ptr;
    if(run_next_id==0){
        PORTD |= _BV(CS_LED); // turn on iddle task LED
    }
    
    return;
}

ISR(TIMER1_COMPA_vect, ISR_NAKED) {
    /* Call the tick function. */
    vPortYieldFromTick();
    PORTD ^= _BV(TICK_LED);
    /* Return from the interrupt. If a context
    switch has occurred this will return to a
    different task. */
    asm volatile ( "reti" );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////// END OF bmOS CODE //////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


TASK(idle, 255, 0, 40, { // lowest priority task will run when no other task can run. This task is always ready.
    asm("nop");
});

TASK(t1, 1, Hz_5, {
    PORTD ^= _BV(2);    // Toggle
    suspend();
});

TASK(t2, 4, Hz_2, {
    PORTD ^= _BV(3);    // Toggle
    suspend();
});

TASK(t3, 10, Hz_1, {
    PORTD ^= _BV(4);    // Toggle
    suspend();
});

int main() {

    addTask(idle); // This task must be the first one for its id to be 0.
    addTask(t1);
    addTask(t2);
    addTask(t3);

    hardwareInit();
    while (true) {
        asm("nop"); // interrrupts will stop this.
    }
}
