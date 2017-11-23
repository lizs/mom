// author : lizs
// 2017.2.21
#ifndef MOM_CIRCULAR_BUF_H
#define MOM_CIRCULAR_BUF_H

#include <exception>
#include <vector>
#include "defines.h"

namespace VK {
	namespace Net {
		class NET_EXPORT CircularBuf final {
			cbuf_len_t m_capacity;
			cbuf_len_t m_reserved;
			cbuf_len_t m_head;
			cbuf_len_t m_tail;
			char* m_buf;
		public:
			explicit CircularBuf();
			~CircularBuf();

			void clear();

			cbuf_len_t get_len() const;
			cbuf_len_t get_writable_len() const;

			bool move_head(cbuf_offset_t offset);
			bool move_tail(cbuf_offset_t offset);

			template <typename T>
			T* get_head_ptr();
			char* get_head_ptr() const;
			char* get_tail_ptr() const;

			template <typename T>
			bool move_tail();

			void set_head(cbuf_len_t pos);
			void set_tail(cbuf_len_t pos);

			cbuf_len_t get_head() const;
			cbuf_len_t get_tail() const;

			void arrange();

			template <typename T>
			void reset();
			void reset(cbuf_len_t size = 0, cbuf_len_t reserved_size = CBUF_RESERVED_SIZE);
			void reuse();

			template <typename T>
			bool write_head(const T& value);

			template <typename T, typename ... Args>
			bool write_head(const T& value, Args ... args);

			template <typename T>
			T* pre_write_head();

			template <typename T>
			T* pre_write();

			bool write(const char* str);

			template <typename T>
			bool write(const T& value);

			template <typename T, typename ... Args>
			bool write(const T& value, Args ... args);

			bool write_binary(char* data, cbuf_len_t len);

			template <typename T>
			bool read(T& out);

			template <typename T>
			bool get(T& out, cbuf_offset_t offset = 0);
		};

		template <typename T>
		bool CircularBuf::move_tail() {
			return move_tail(sizeof(T));
		}

		template <typename T>
		T* CircularBuf::get_head_ptr() {
			return reinterpret_cast<T*>(m_buf + m_head);
		}

		template <typename T>
		void CircularBuf::reset() {
			reset(sizeof(T));
		}

		template <typename T>
		bool CircularBuf::write_head(const T& value) {
			if (m_head < sizeof(T))
				return false;

			if (!move_head(-static_cast<cbuf_offset_t>(sizeof(T)))) return false;
			T* p = reinterpret_cast<T*>(get_head_ptr());
			*p = value;
			return true;
		}

		template <typename T, typename ... Args>
		bool CircularBuf::write_head(const T& value, Args ... args) {
			return write_head(value) && write_head(args ...);
		}

		template <typename T>
		T* CircularBuf::pre_write_head() {
			if (m_head < sizeof(T))
				return nullptr;

			if (!move_head(-static_cast<cbuf_offset_t>(sizeof(T))))
				return nullptr;

			return reinterpret_cast<T*>(get_head_ptr());
		}

		template <typename T>
		T* CircularBuf::pre_write() {
			if (get_writable_len() < sizeof(T))
				return nullptr;

			T* p = reinterpret_cast<T*>(get_tail_ptr());
			if (!move_tail(sizeof(T)))
				return nullptr;

			return p;
		}

		template <typename T>
		bool CircularBuf::write(const T& value) {
			if (get_writable_len() < sizeof(T))
				return false;

			T* p = reinterpret_cast<T*>(get_tail_ptr());
			*p = value;

			return move_tail(sizeof(T));
		}

		template <typename T, typename ... Args>
		bool CircularBuf::write(const T& value, Args ... args) {
			return write(value) && write(args ...);
		}

		template <typename T>
		bool CircularBuf::read(T& out) {
			if (get_len() < sizeof(T))
				return false;

			out = *reinterpret_cast<T*>(get_head_ptr());
			return move_head(sizeof(T));
		}

		template <typename T>
		bool CircularBuf::get(T& out, cbuf_offset_t offset) {
			if (offset < 0 || m_head + offset + sizeof(T) > m_tail)
				return false;

			out = *reinterpret_cast<T*>(get_head_ptr() + offset);
			return true;
		}

#pragma region allocator
		NET_EXPORT cbuf_ptr_t alloc_cbuf(cbuf_len_t len);

		template <typename T>
		static cbuf_ptr_t alloc_cbuf() {
			return alloc_cbuf(sizeof(T));
		}

		template <typename ... Args>
		static std::vector<cbuf_ptr_t> pack(cbuf_ptr_t pcb, Args ... args) {
			auto ret = std::vector<cbuf_ptr_t>();

			// 1 pattern
			// 2 serial ...
			if (!pcb->write_head(args...))
				return ret;

			cbuf_len_t limit = MAX_SINGLE_PACKAGE_SIZE - sizeof(byte_t);
			if (pcb->get_len() > limit) {
				// multi
				auto cnt = pcb->get_len() / limit;
				auto lastLen = pcb->get_len() % limit;
				if (lastLen != 0)
					++cnt;

				if (cnt > MAX_SLICE_COUNT) {
					return ret;
				}

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

			// single
			if (!pcb->write_head<byte_t>(1))
				return ret;

			if (!pcb->write_head<pack_size_t>(pcb->get_len()))
				return ret;

			ret.push_back(pcb);
			return std::move(ret);
		}
#pragma endregion
	}
}

#endif
