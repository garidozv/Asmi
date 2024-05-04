#ifndef HELPER_HPP
#define HELPER_HPP


#include <string>


class Helper {
public:
	static std::string* make_string(const char* s);
	static std::string* copy_string(std::string* s);
	static std::string* concat_strings_with_comma(std::string* s1, std::string* s2);
};


#endif
