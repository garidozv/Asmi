CXX = g++
CXXFLAGS = -g  -pthread -o
FILES = main.cpp Emulator.cpp
MISC = ../elf/Elf32File.cpp

prog: $(FILES) $(MISC) 
	$(CXX) $(CXXFLAGS) $@ $^

test:
	./prog ../linker/program.hex