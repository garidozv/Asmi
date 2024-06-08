ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

DIR=./tests/test-equ

${ASSEMBLER} -o file1.o ${DIR}/file1.s
${ASSEMBLER} -o file2.o ${DIR}/file2.s

${LINKER} -hex \
  -place=code@0x40000000 \
  -o program.hex \
  file1.o file2.o

