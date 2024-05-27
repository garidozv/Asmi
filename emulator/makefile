CXX = g++
CXXFLAGS = -g -o
FILES = main.cpp Emulator.cpp
MISC = ../elf/Elf32File.cpp

prog: $(MISC) $(FILES)
	$(CXX) $(CXXFLAGS) $@ $^

test:
	./prog ../linker/program.hex