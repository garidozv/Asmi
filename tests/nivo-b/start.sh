ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

DIR=tests/nivo-b

${ASSEMBLER} -o main.o ${DIR}/main.s
${ASSEMBLER} -o handler.o ${DIR}/handler.s
${ASSEMBLER} -o isr_terminal.o ${DIR}/isr_terminal.s
${ASSEMBLER} -o isr_timer.o ${DIR}/isr_timer.s
${LINKER} -hex \
  -place=my_code@0x40000000 \
  -o program.hex \
  main.o isr_terminal.o isr_timer.o handler.o
${EMULATOR} program.hex