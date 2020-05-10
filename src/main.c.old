#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>

#define TASK_BINS 20

#define Hz_10 6250  // compare match register 16MHz/256/10Hz
#define Hz_2  31250 // compare match register 16MHz/256/2Hz
#define Hz_2k 31    // compare match register 16MHz/256/2kHz

/* 4 Tasks:
 *     T1 - T = 100ms   -> Led PD3 toggle
 *     T2 - T = 200ms   -> Led PD4 toggle
 *     T3 - T = 600ms   -> Led PD5 toggle
 *     T4 - T = 100ms   -> Led PD6 copied from button A1
 */

void t1_init() {
  pinMode(PD3, OUTPUT);
}
void t2_init() {
  pinMode(PD4, OUTPUT);
}
void t3_init() {
  pinMode(PD5, OUTPUT);
}
void t4_init() {
  pinMode(PD6, OUTPUT);
}
void t5_init() {
  pinMode(PD7, OUTPUT);
}

void t1() {
  digitalWrite(PD3, !digitalRead(PD3));
}
void t2() {
  digitalWrite(PD4, !digitalRead(PD4));
}
void t3() {
  digitalWrite(PD5, !digitalRead(PD5));
}
void t4() {
  digitalWrite(PD6, !digitalRead(PD6));
}
void t5() {
  digitalWrite(PD7, !digitalRead(PD7));
}

typedef struct {
  int period;     /* period in ticks */
  int delay;      /* ticks until next activation */
  int exec;       /* activation counter */
  void (*func)(); /* function pointer */
} Sched_Task_t;

Sched_Task_t Task_List[TASK_BINS];

/**
 * @brief Initialise data structures.
 * Configure interrupt that periodically calls Sched_Schedule().
 *
 */
void Sched_Init(int comp) {
  for (int x = 0; x < TASK_BINS; x++) {
    Task_List[x].func = 0;
  }

  /* Also configures interrupt that periodically calls Sched_Schedule(). */
  noInterrupts();  // disable all interrupts

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
 
  OCR1A = comp;
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS12); // 256 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

  interrupts();  // enable all interrupts
}

int Sched_AddT(void (*f)(), int d, int p) {
  for (int x = 0; x < TASK_BINS; x++) {
    if (!Task_List[x].func) {
      Task_List[x].period = p;
      Task_List[x].delay = d;
      Task_List[x].exec = 0;
      Task_List[x].func = f;
      return x;
    }
  }

  return -1;
}

/**
 * @brief Verifies if any task needs to be activated, and if so, increments by 1
 * the task's pending activation counter.
 *
 */
void Sched_Schedule() {
  for (int x = 0; x < TASK_BINS; x++) {
    if (!Task_List[x].func) {  // deleted tasks have func set to NULL
      continue;
    }

    if (Task_List[x].delay) {
      Task_List[x].delay--;
    } else {
      /* Schedule Task */
      Task_List[x].exec++;
      Task_List[x].delay = Task_List[x].period - 1;
    }
  }
}

/**
 * @brief Verifies if any task has an activation counter > 0, and if so, calls
 * that task.
 *
 */
void Sched_Dispatch() {
  for (int x = 0; x < TASK_BINS; x++) {
    if (Task_List[x].exec) {
      Task_List[x].exec--;
      Task_List[x].func();
      /* Delete task if one-shot */
      if (!Task_List[x].period) {
        Task_List[x].func = NULL;
      }
    }
  }
}

volatile int go = 0;

ISR(TIMER1_COMPA_vect) {
  t4();
  Sched_Schedule();
}

int main() {
  // initialize the kernel
  Sched_Init(Hz_2);
  t1_init();
  t2_init();
  t3_init();
  t4_init();
  t5_init();
  /* periodic task */
  Sched_AddT(t1, 2, 4);
  Sched_AddT(t2, 2, 8);
  Sched_AddT(t5, 0, 1);
  /* one-shot task */
  Sched_AddT(t3, 50, 0);

  while (true) {
    Sched_Dispatch();
  }
}
