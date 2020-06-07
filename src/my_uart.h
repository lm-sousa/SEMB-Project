#ifndef BAUD
    #define BAUD 9600
#endif

#ifndef F_CPU
    #define F_CPU 8000000UL
#endif

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>


void uart_init();

void uart_putchar(char c);

void uart_putstr(char *data);

void uart_putul(uint32_t u);