.extern isr_timer, isr_terminal, isr_inv, isr_int

.global handler
.section my_handler
handler:
    push %r1
    push %r2
    csrrd %cause, %r1
    ld $1, %r2
    beq %r1, %r2, handle_inv
    ld $2, %r2
    beq %r1, %r2, handle_timer
    ld $3, %r2
    beq %r1, %r2, handle_terminal
    ld $4, %r2
    beq %r1, %r2, handle_int
finish:
    pop %r2
    pop %r1
    iret
# obrada prekida od tajmera
handle_timer:
    call isr_timer
    jmp finish
# obrada prekida od terminala
handle_terminal:
    call isr_terminal
    jmp finish
# obrada softverskog prekida
handle_int:
    call isr_int
    jmp finish
# obrada prekida usljed nevalidne instrukcije
handle_inv:
    call isr_inv
    jmp finish
    
.end
