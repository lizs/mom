// lizs 2017.6.17

#include <string.h>
#include <sstream>
#include "util/str.h"

namespace VK {
	std::vector<std::string> split(const std::string& input, char delim, const std::string& except) {
		return split(input, std::vector<char>{delim}, except);
	}

	std::vector<std::string> split(const std::string& input, const std::vector<char>& delim, const std::string& except) {
		std::vector<std::string> ret;

		std::string item;
		for (auto c : input) {
			if (std::find(delim.begin(), delim.end(), c) == delim.end())
				item += c;
			else {
				if (item != except)
					ret.push_back(item);
				item.clear();
			}
		}

		if (item != except)
			ret.push_back(item);
		return ret;
	}

	//std::vector<std::string> split(char* input, char* delim) {
	//	std::vector<std::string> ret;

	//	char* p = nullptr;
	//	p = strtok(input, delim);
	//	if (!p) {
	//		return ret;
	//	}

	//	if (strlen(p) != 0) {
	//		ret.push_back(p);
	//	}

	//	while ((p = strtok(nullptr, delim))) {
	//		if (strlen(p) != 0) {
	//			ret.push_back(p);
	//		}
	//	}

	//	return ret;
	//}

	std::string join(std::vector<std::string>& items, char delim) {
		std::stringstream ss;
		for (auto i = 0; i < items.size(); ++i) {
			ss << items[i];
			if (i != items.size() - 1) {
				ss << delim;
			}
		}

		return ss.str();
	}
}