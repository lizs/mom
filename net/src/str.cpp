// lizs 2017.6.17

#include <string.h>
#include <sstream>
#include "str.h"

namespace VK {
	std::vector<std::string> split(char* input, char* delim) {
		std::vector<std::string> ret;

		char* p = nullptr;
		p = strtok(input, delim);
		if (!p) {
			return ret;
		}

		if (strlen(p) != 0) {
			ret.push_back(p);
		}

		while ((p = strtok(nullptr, delim))) {
			if (strlen(p) != 0) {
				ret.push_back(p);
			}
		}

		return ret;
	}

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