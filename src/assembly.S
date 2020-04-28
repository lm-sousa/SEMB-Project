// Remember that every assembly source file
// must have the suffix .S
// (upper case) since it is processed by GCC
// and NOT the Atmel assembler
#define _SFR_ASM_COMPAT 1
// this is needed for every assembly source
// file so we can use
// the IO names as usual
#define __SFR_OFFSET 0
// this is needed for every assembly source
// file so we can use IO with IN and OUT
// statementsa
#include <avr/io.h>
// since this file is processed by the
// C-preprocessor
// comments after #include or #define
// must be preceded by //
; afterwards we can use the traditional
; semicolon
.global asmfunction ; all assembly labels
 ; which are called
 ; from C need to be defined global
asmfunction: ; entry point for
 ; assembly function
 out PORTB,r24
 call cfunction
 ; cfunction does not need to be
 ; declared here, the linker will find
 ; it (no type checking however)
 out PORTB,r23
 lds r14,variable
 out PORTB,r22
 mov r24,r14
 ret
.global SIG_OVERFLOW1
 SIG_OVERFLOW1:
 out PORTB,r20
 out PORTB,r19
 reti