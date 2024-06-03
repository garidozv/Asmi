
.global handler
.extern isr_timer

.section my_handler
handler:
    push %r1
    push %r2
    csrrd %cause, %r1
    ld $2, %r2
    beq %r1, %r2, handle_timer
finish:
    pop %r2
    pop %r1
    iret
# obrada prekida od tajmera
handle_timer:
    call isr_timer
    jmp finish
    
.end
