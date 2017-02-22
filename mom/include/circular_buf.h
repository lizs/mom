// author : lizs
// 2017.2.21
#pragma once
#include <exception>

namespace Bull {
	template <ushort Capacity = 1024>
	class CircularBuf {
	public:
		explicit CircularBuf()
			: m_head(0),
			  m_tail(0) { }

		ushort get_len() const {
			if (m_tail < m_head) throw std::exception("get_len");
			return m_tail - m_head;
		}

		ushort get_writable_len() const {
			if (Capacity < m_tail) throw std::exception("get_writable_len");
			return Capacity - m_tail;
		}

		ushort get_readable_len() const {
			if (m_tail < m_head) throw std::exception("get_readable_len");
			return m_tail - m_head;
		}

		void move_all(short offset) {
			move_head(offset);
			move_tail(offset);
		}

		void move_head(short offset) {
			m_head += offset;
			if (m_head < 0 || m_head > Capacity)
				throw std::exception("move_head");
		}

		void move_tail(short offset) {
			m_tail += offset;
			if (m_tail < 0 || m_tail > Capacity)
				throw std::exception("move_tail");
		}

		char* get_head() {
			return m_buf + m_head;
		}

		char* get_tail() {
			return m_buf + m_tail;
		}

		void arrange() {
			if (m_tail * 1.2 > Capacity) {
				auto len = get_len();
				memcpy(m_buf, get_head(), len);

				m_head = 0;
				m_tail = len;
			}
		}

		void reset() {
			m_head = 0;
			m_tail = 0;
			ZeroMemory(m_buf, Capacity);
		}

		template <typename T>
		void write(T value, short offset) {
			move_tail(offset);
			if (get_writable_len() < sizeof(T))
				throw std::exception("write");

			T* p = reinterpret_cast<T*>(get_tail());
			*p = value;

			move_tail(sizeof(T));
		}

		template <typename T>
		void write(T value) {
			write<T>(value, 0);
		}

		void write_binary(char* data, ushort len, ushort offset) {
			move_tail(offset);
			if (get_writable_len() < len)
				throw std::exception("write");

			memcpy(get_tail(), data, len);
			move_tail(len);
		}

		void write_binary(char* data, ushort len) {
			write_binary(data, len, 0);
		}

		template <typename T>
		T read() {
			if (get_readable_len() < sizeof(T))
				throw std::exception("read");

			T ret = *reinterpret_cast<T*>(get_head());
			move_head(sizeof(T));
			return ret;
		}

	private:
		ushort m_head;
		ushort m_tail;

		// use array to avoid heap allocation
		char m_buf[Capacity];
	};

}
