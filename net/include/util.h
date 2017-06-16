// lizs 2017.3.8
#pragma once
#include "defines.h"
#include "circular_buf.h"
#include <vector>

namespace VK {
	namespace Net {
		NET_EXPORT bool make_dir(const char* relative);
		NET_EXPORT bool make_dir(const std::string & relative);
		NET_EXPORT const char* net_str_err(error_no_t error_no);
		NET_EXPORT cbuf_ptr_t alloc_cbuf(cbuf_len_t len);
		NET_EXPORT std::vector<std::string> split(char* input, char* delim);
		NET_EXPORT std::string join(std::vector<std::string>& items, char delim);

		template <typename T>
		static cbuf_ptr_t alloc_cbuf() {
			return alloc_cbuf(sizeof(T));
		}

		// 打包
		template <typename ... Args>
		static std::vector<cbuf_ptr_t> pack(cbuf_ptr_t pcb, Args ... args) {
			auto ret = std::vector<cbuf_ptr_t>();

			// 1 pattern
			// 2 serial ...
			if (!pcb->write_head(args...))
				return ret;

			cbuf_len_t limit = MAX_PACKAGE_SIZE - sizeof(byte_t);
			if (pcb->get_len() > limit) {
				// 多包
				auto cnt = pcb->get_len() / limit;
				auto lastLen = pcb->get_len() % limit;
				if (lastLen != 0)
					++cnt;

				if (cnt == 1)
					goto Single;

				for (auto i = 0; i < cnt; ++i) {
					cbuf_ptr_t slice;
					auto packLen = limit;
					if (i == cnt - 1 && lastLen != 0)
						packLen = lastLen;

					slice = alloc_cbuf(packLen);
					slice->write_binary(pcb->get_head_ptr() + i * limit, packLen);

					if (!slice->write_head<byte_t>(cnt - i))
						return ret;

					if (!slice->write_head<pack_size_t>(slice->get_len()))
						return ret;

					ret.push_back(slice);
				}

				return ret;
			}
			else {
Single:
				// 单包
				if (!pcb->write_head<byte_t>(1))
					return ret;

				if (!pcb->write_head<pack_size_t>(pcb->get_len()))
					return ret;

				ret.push_back(pcb);
			}

			return std::move(ret);
		}
	}
}
