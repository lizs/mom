// lizs 2017.6.17
#pragma once
#include "defines.h"
#include <vector>
#include <string>

namespace VK {
	NET_EXPORT std::vector<std::string> split(const std::string& input, const std::vector<char>& delim, const std::string& except = "");
	NET_EXPORT std::vector<std::string> split(const std::string& input, char delim, const std::string& except = "");
	NET_EXPORT std::string join(std::vector<std::string>& items, char delim);
}
