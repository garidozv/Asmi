ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

DIR=./tests/test1

${ASSEMBLER} -o main.o ${DIR}/main.s
${ASSEMBLER} -o handler.o ${DIR}/handler.s
${ASSEMBLER} -o isr_timer.o ${DIR}/isr_timer.s
${ASSEMBLER} -o print_string.o ${DIR}/print_string.s
${LINKER} -hex \
  -place=code@0x40000000 \
  -place=print@0x80808080 \
  -o program.hex \
  main.o print_string.o handler.o isr_timer.o
${EMULATOR} program.hex
