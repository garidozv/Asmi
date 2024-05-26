CXX = g++
CXXFLAGS = -g -o
FILES = main.cpp
MISC = ../Helper.cpp Linker.cpp ../elf/Elf32File.cpp

prog: $(MISC) $(FILES)
	$(CXX) $(CXXFLAGS) $@ $^

test:
	./prog -o program.hex -hex -place=my_code@0x40000000 -place=math@0x40000100 ../results/handler.o ../results/math.o ../results/main.o  ../results/isr_terminal.o ../results/isr_timer.o  ../results/isr_software.o