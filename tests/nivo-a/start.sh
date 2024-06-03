ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

DIR=tests/nivo-a

${ASSEMBLER} -o main.o ${DIR}/main.s
${ASSEMBLER} -o math.o ${DIR}/math.s
${ASSEMBLER} -o handler.o ${DIR}/handler.s
${ASSEMBLER} -o isr_timer.o ${DIR}/isr_timer.s
${ASSEMBLER} -o isr_terminal.o ${DIR}/isr_terminal.s
${ASSEMBLER} -o isr_software.o ${DIR}/isr_software.s
${LINKER} -hex \
  -place=my_code@0x40000000 -place=math@0xF0000000 \
  -o program.hex \
  handler.o math.o main.o isr_terminal.o isr_timer.o isr_software.o
${EMULATOR} program.hex