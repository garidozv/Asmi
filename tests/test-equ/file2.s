.extern a
.global x

.equ p, q + g
.equ q, g + l1 - l2
.equ g, 20

.section code_2
l1:
  ld $0xfff, %r1
  ld 2, %r2
  add %r1, %r2
l2:
  ret

.section data_2
.word a
x:

.end
