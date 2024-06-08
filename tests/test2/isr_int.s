.global isr_int
.extern print_string

.equ message_addr, message_int

.section isr
isr_int:
  push %r1
  ld $message_addr, %r1
  call print_string
  pop %r1
  ret

.section strings
message_int: .ascii "Software interrupt!\n\0"

.end
