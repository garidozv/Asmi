ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

DIR=./tests/test2

${ASSEMBLER} -o main.o ${DIR}/main.s
${ASSEMBLER} -o handler.o ${DIR}/handler.s
${ASSEMBLER} -o isr_terminal.o ${DIR}/isr_terminal.s
${ASSEMBLER} -o isr_timer.o ${DIR}/isr_timer.s
${ASSEMBLER} -o isr_int.o ${DIR}/isr_int.s
${ASSEMBLER} -o isr_inv.o ${DIR}/isr_inv.s
${ASSEMBLER} -o print_string.o ${DIR}/print_string.s
${ASSEMBLER} -o read_char.o ${DIR}/read_char.s

${LINKER} -relocatable \
  -place=print@0x10000000 \
  -o io.o \
  print_string.o read_char.o

${LINKER} -hex \
  -place=code@0x40000000 \
  -place=isr@0x80001000 \
  -o program.hex \
  main.o handler.o isr_terminal.o isr_timer.o isr_int.o isr_inv.o io.o

${EMULATOR} program.hex