ASSEMBLER=../../assembler/prog
LINKER=../../linker/prog
EMULATOR=../../emulator/prog

${ASSEMBLER} -o main.o main.s
${ASSEMBLER} -o handler.o handler.s
${ASSEMBLER} -o isr_timer.o isr_timer.s
${ASSEMBLER} -o print_string.o print_string.s
${LINKER} -hex \
  -place=code@0x40000000 \
  -place=print@0x80808080 \
  -o program.hex \
  main.o print_string.o handler.o isr_timer.o
${EMULATOR} program.hex