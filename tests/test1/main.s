
.extern print_string, handler

.equ initial_sp, 0xFFFFFEFE
.equ timer_config, 0xFFFFFF10
.section code
my_start:
    ld $initial_sp, %sp
    ld $handler, %r1
    csrwr %r1, %handler

    ld $0x1, %r1
    st %r1, timer_config

    ld $hello, %r1
    call print_string

    ld $str, %r1
    call print_string

    #ld $1000000, %r1
    #ld $1, %r2
#loop:
    #sub %r2, %r1
    #bne %r1, %r0, loop

    halt

.section strings
hello:
  .ascii "Hello World\n\0"
str:
  .ascii "What's up?\n\0"

.end
