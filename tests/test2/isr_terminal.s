# file: isr_terminal.s

.extern flag, char

.section isr
# prekidna rutina za terminal
.equ terminal_out, 0xFFFFFF00
.equ terminal_in, 0xFFFFFF04
.global isr_terminal
isr_terminal:
    push %r1
    push %r2

    ld terminal_in, %r1
    st %r1, char
    ld $1, %r2
    st %r2, flag

    pop %r2
    pop %r1
    ret
  
.end
