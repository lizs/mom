// lizs 2017.6.17
#pragma once
#include "defines.h"
#include <vector>
#include <string>

namespace VK {
	NET_EXPORT std::vector<std::string> split(char* input, char* delim);
	NET_EXPORT std::string join(std::vector<std::string>& items, char delim);
}
