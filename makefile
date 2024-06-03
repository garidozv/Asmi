CXX = g++
CXXFLAGS = -g -o
EMUFLAGS = -pthread
YFLAGS = -d -t -o
LFLAGS = --header-file=$(MISCDIR)/lexer.hpp -o

ASM = assembler
LNK = linker
EMU = emulator

ASMDIR = ./src/$(ASM)
LNKDIR = ./src/$(LNK)
EMUDIR = ./src/$(EMU)
ELFDIR = ./src/elf
MISCDIR = ./misc
HLPDIR = ./src

LFILE = $(MISCDIR)/lexer.cpp
YFILE = $(MISCDIR)/parser.cpp
HLPFILE = $(HLPDIR)/Helper.cpp

all: $(ASM) $(LNK) $(EMU)

$(ASM): $(LFILE) $(YFILE) $(wildcard $(ASMDIR)/*.cpp) $(wildcard $(ELFDIR)/*.cpp) $(HLPFILE)
	$(CXX) $(CXXFLAGS) $@ $^

$(LNK): $(wildcard $(LNKDIR)/*.cpp) $(wildcard $(ELFDIR)/*.cpp) $(HLPFILE)
	$(CXX) $(CXXFLAGS) $@ $^

$(EMU): $(wildcard $(EMUDIR)/*.cpp) $(wildcard $(ELFDIR)/*.cpp)
	$(CXX) $(EMUFLAGS) $(CXXFLAGS) $@ $^

$(LFILE): $(MISCDIR)/lexer.l
	flex $(LFLAGS) $@ $^

$(YFILE): $(MISCDIR)/parser.y
	bison $(YFLAGS) $@ $^


clean: temp_clear
	rm -f $(wildcard $(MISCDIR)/*.cpp) $(wildcard $(MISCDIR)/*.hpp)
	rm $(ASM) $(LNK) $(EMU)

temp_clear:
	rm -f *.o *.readelf *.hex*

.SILENT: temp_clear
.PHONY: all clean clean_temp $(ASM) $(LNK) $(EMU) 