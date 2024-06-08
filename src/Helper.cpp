#include "../inc/Helper.hpp"


std::string* Helper::make_string(const char* s) {
	return new std::string(s);
}

std::string* Helper::copy_string(std::string* s) {
	return new std::string(*s);
}

std::string* Helper::concat_strings_with_comma(std::string* s1, std::string* s2) {
	return new std::string(*s1 + "," + *s2);
}

bool Helper::checkSymbolicList(std::string* list) {
    std::vector<std::string> elems = splitString(*list, ',');
    for ( std::string elem : elems ) {
        if (Helper::isNumber(elem) ) return false; 
    }
    return true;
}

bool Helper::isNumber(std::string string) {
    return std::isdigit(string[0]) || string[0] == '-';
}


int  Helper::parseReg(std::string reg) {
    if ( reg == "status" ) return 16;
    else if ( reg == "handler" ) return 17;
    else if ( reg == "cause" ) return 18;
    else if ( reg == "sp" ) return 14;
    else if ( reg == "pc" ) return 15;
    else return std::atoi(reg.substr(1).c_str());
}

std::string Helper::regToString(int reg) {
    if ( reg > 15 ) {
            switch (reg)
            {
            case 16: return "status";
            case 17: return "handler";
            case 18: return "cause";
            }
    } else if ( reg >= 0 ) return ("r" + std::to_string(reg));

    return "";
}

std::vector<std::string> Helper::splitString(std::string str, char delim) {
    std::vector<std::string> tokens;
    std::string temp;
    std::istringstream stream(str);
    while (std::getline(stream, temp, delim)) {
        tokens.push_back(temp);
    }

    return tokens;
}

void Helper::printHex(std::ostream& os, uint32_t number, int width, bool prefix) {
    std::ios old_state(nullptr);
    old_state.copyfmt(os);

    if ( prefix ) os << std::showbase;
    os << std::hex << std::internal << std::setfill('0');

    // We do this because showbase wont work if number is 0
    if ( prefix && number == 0 ) os << "0x" << std::setw(width - 2) << "";
    else os << std::setw(width) << number;

    os.copyfmt(old_state);
}