#include "freq16_256.h"

/*****************************************************************************/
/******************************* CONFIGURATION *******************************/
/*****************************************************************************/
#define TICK_FREQUENCY      Hz_5
#define STACK_SIZE_DEFAULT  100
#define MAX_TASKS           3
#define NUMBER_OF_MUTEXES   8
/*****************************************************************************/

// Include kernel code
#include "bmOS.h" // DO NOT MOVE THIS LINE


/**************************** ADD YOUR TASKS HERE ****************************/

/***
 * TASK | FREQUENCY | TASK_FREQUENCY 
 * T1   | 5Hz       | 2
 * T2   | 2Hz       | 5
 * T3   | 1Hz       | 10
 */

TASK(t1, 1, Hz_5, {
    PORTD ^= _BV(2);    // Toggle
    suspend();
});

TASK(t2, 2, Hz_2, 0, {
    PORTD ^= _BV(3);    // Toggle
    suspend();
});

TASK(t3, 3, Hz_1, 0, {
    PORTD ^= _BV(4);    // Toggle
    suspend();
});

/***************************** END OF TASKS CODE *****************************/

kernel_main({
    
    /********************** CONFIGURE REQUIRED HARDWARE **********************/

    DDRD |= _BV(2);
    DDRD |= _BV(3);
    DDRD |= _BV(4);

    /********************* END OF HARDWARE CONFIGURATION *********************/
    /********************* REGISTER TASKS IN THE KERNEL **********************/

    addTask(t1);
    addTask(t2);
    addTask(t3);

    /***************************** END OF CODE *******************************/
})
