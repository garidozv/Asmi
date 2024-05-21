CXX = g++
YFLAGS = -d -t
LFILE = lexer.cpp
YFILE = parser.cpp
FILES = Assembler.cpp
MISC = Helper.cpp

prog: $(LFILE) $(YFILE) $(MISC) $(FILES)
	$(CXX) -o $@ $^

$(LFILE): lexer.l
	flex $^

$(YFILE): parser.y
	bison $(YFLAGS) $^

test:
	cat sample2.txt | ./prog
	#make -s clean

clean:
	rm -f lexer.cpp lexer.hpp
	rm -f parser.cpp parser.hpp
	rm prog


.SILENT:clean
