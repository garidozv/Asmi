.extern handler, read_char
.extern char, print_string

.global wait_flag

.equ initial_sp, 0xFFFFFEFE
.equ timer_config, 0xFFFFFF10

.section code
  ld $initial_sp, %sp
  ld $handler, %r1
  csrwr %r1, %handler

  ld $0x4, %r1
  st %r1, timer_config

  int

  call read_char
  ld char, %r10
  ld $res, %r11
  # since st and ld work with 4B words, and we only want to write 1B character
  # we will have to first load a word from target address, do bitwise OR with our char and write it back
  ld [%r11 + 0], %r12
  or %r12, %r10
  st %r10, [%r11 + 0]

loop:
  # wait 5 secs
  ld wait_flag, %r1
  beq %r0, %r1, loop

  call read_char
  ld char, %r10
  ld $res, %r11
  ld [%r11 + 1], %r12
  or %r12, %r10
  st %r10, [%r11 + 1]

  # invalid instruction

  ld $res, %r1
  call print_string

  .word 0x01642021

  halt



.section data
wait_flag: .skip 4

.section strings
res: .word 0x000a0000 # c1 c2 \n \0 - in reverse order because word will be written in little endian


.end
