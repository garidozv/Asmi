.global a
.extern x

.equ a, 0xffff
.equ b, lab + 4

.equ c, second - first + 0x100 + x

.section code
  ld $2, %r1
lab:
  ld $c, %r1
  ld $b, %r3
  halt


.section data
first: .skip 4
second: .skip 10
third: .skip 2


.end
