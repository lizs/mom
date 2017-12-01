// lizs 2017.6.17
#include "util/file.h"
#include "util/str.h"

namespace VK {
	bool make_dir(const std::string & relative) {
		char path[256] = {0};
		strncpy(path, relative.c_str(), sizeof(path));

		return make_dir(path);
	}


	bool make_dir(const char* relative) {
		auto items = std::move(split(const_cast<char*>(relative), {'/', '\\'}));
		if (items.empty()) {
			return false;
		}

		auto loop = uv_default_loop();
		if (loop == nullptr)
			return false;

		uv_fs_t req;
		auto ret = 0;
		std::string cur;
		for (auto & item : items) {
			cur += item;
			cur += "/";

			ret = uv_fs_mkdir(loop, &req, cur.c_str(), 0, nullptr);
			if (ret && ret != UV_EEXIST) {
				printf("%s", uv_strerror(ret));

				return false;
			}
		}

		return true;
	}

}
