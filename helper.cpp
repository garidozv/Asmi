#include "helper.hpp"


std::string* Helper::make_string(const char* s) {
	return new std::string(s);
}

std::string* Helper::copy_string(std::string* s) {
	return new std::string(*s);
}

std::string* Helper::concat_strings_with_comma(std::string* s1, std::string* s2) {
	return new std::string(*s1 + "," + *s2);
}
