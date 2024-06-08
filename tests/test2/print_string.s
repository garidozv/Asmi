 .global print_string

.equ term_out, 0xffffff00
.equ char_mask, 0xff

.section print
print_string:
  # r1 - pocetna adresa stringa
  # cita se do '\0'
  push %r1
  push %r2
  push %r3
  ld $char_mask, %r3

load:  ld [%r1], %r2
  and %r3, %r2
  beq %r0, %r2, end
  st %r2, term_out
  ld $1, %r2
  add %r2, %r1
  jmp load 

end:

  pop %r3
  pop %r2
  pop %r1

  ret


.end
