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
asmfunction:
    in r30, PORTB
    inc r30
    out PORTB, r30
    clr r24
    inc r24
    inc r24
    clr r20
    inc r20
    inc r20
    call cfunction
    out PORTB, r24
    ret
