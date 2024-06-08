# file: isr_timer.s

.extern wait_flag

.section isr
.global isr_timer
isr_timer:
    push %r1
    ld $1, %r1
    
    st %r1, wait_flag

    pop %r1
    ret

.end
