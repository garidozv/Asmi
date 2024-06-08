
.global read_char, flag, char

.section read
read_char:
  push %r1
  push %r2

  ld $0x1, %r1
loop:
  ld flag, %r2
  and %r1, %r2
  bne %r1, %r2, loop
  st %r0, flag
  pop %r2
  pop %r1
  ret


.section data
flag: .skip 4
char: .word 0

.end
