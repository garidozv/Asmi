#ifndef HELPER_H
#define HELPER_H


#include <string>
#include <vector>
#include <sstream>


class Helper {
public:
	static std::string* make_string(const char* s);
	static std::string* copy_string(std::string* s);
	static std::string* concat_strings_with_comma(std::string* s1, std::string* s2);
	static int parseReg(std::string reg);
	static std::string regToString(int reg);
	static std::vector<std::string> splitString(std::string str, char delim);
	static bool checkSymbolicList(std::string* s);
};


#endif
