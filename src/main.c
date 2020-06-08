#include "freq16_256.h"

/*****************************************************************************/
/******************************* CONFIGURATION *******************************/
/*****************************************************************************/
#define TICK_FREQUENCY      Hz_20
#define STACK_SIZE_DEFAULT  100
#define MAX_TASKS           3
#define NUMBER_OF_MUTEXES   8
/*****************************************************************************/

// Include kernel code
#include "bmOS.h" // DO NOT MOVE THIS LINE



/**************************** ADD YOUR TASKS HERE ****************************/

TASK(t1, 1, Hz_1, {
    PORTD ^= _BV(2);    // Toggle
    suspend();
});

uint64_t cnt = 0;
TASK(t2, 4, Hz_1, 0, {
    PORTD ^= _BV(3);    // Toggle
    for (; cnt < 1000000000; cnt++) {
        asm("nop");
    }
    cnt = 0;
    PORTD ^= _BV(3);    // Toggle
    suspend();
});

TASK(t3, 5, Hz_2, 0, {
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
