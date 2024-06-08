.global isr_inv, message_addr
.extern print_string

.equ message_addr, message_inv

.section isr
isr_inv:
  push %r1
  ld $message_addr, %r1
  call print_string
  pop %r1
  ret

.section strings
message_inv: .ascii "Invalid instruction!\n\0"

.end
