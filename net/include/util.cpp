#include "util.h"
#include "_singleton_.inl"
#include "mem_pool.h"
#include "logger.h"
#include <sstream>

namespace VK {
	namespace Net {
		cbuf_ptr_t alloc_cbuf(cbuf_len_t len) {
			cbuf_ptr_t pcb(_singleton_<cbuf_pool_t>::instance().alloc());
			pcb->reset(len);

			return pcb;
		}

		bool make_dir(const std::string & relative) {
			char path[256] = { 0 };
			strncpy(path, relative.c_str(), sizeof(path));

			return make_dir(path);
		}


		bool make_dir(const char* relative) {
			auto items = std::move(Net::split(const_cast<char*>(relative), "/\\"));
			if (items.empty()) {
				return false;
			}
			
			auto loop = uv_default_loop();
			if (loop == nullptr)
				return false;
			
			uv_fs_t req;
			auto ret = 0;
			std::string cur;
			for(auto & item : items) {
				cur += item;
				cur += "/";

				ret = uv_fs_mkdir(loop, &req, cur.c_str(), 0, nullptr);
				if (ret && ret != UV_EEXIST) {
					printf(uv_strerror(ret));

					return false;
				}
			}

			return true;
		}

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

		const char* net_str_err(error_no_t error_no) {
			switch (static_cast<NetError>(error_no)) {
			case Success: return "Success.";
			case NE_Write: return "Write failed.";
			case NE_Read: return "Read failed.";
			case NE_SerialConflict: return "Serial failed.";
			case NE_NoHandler: return "No handler.";
			case NE_ReadErrorNo: return "Read errno failed.";
			case NE_SessionClosed: return "Session closed.";
			case NE_End: return "NE_End.";
			default: return "Unknown";
			}
		}
	}
}
