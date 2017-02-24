// author : lizs
// 2017.2.21
#pragma once
#include <exception>
#include "bull.h"

namespace Bull {
	const cbuf_len_t ReserverdSize = 8;

	template <cbuf_len_t Capacity>
	class CircularBuf {
	public:
		explicit CircularBuf()
			: m_head(0),
			  m_tail(0) { }

		cbuf_len_t get_len() const {
			if (m_tail < m_head) throw std::exception("get_len");
			return m_tail - m_head;
		}

		cbuf_len_t get_writable_len() const {
			if (Capacity < m_tail) throw std::exception("get_writable_len");
			return Capacity - m_tail;
		}

		cbuf_len_t get_readable_len() const {
			if (m_tail < m_head) throw std::exception("get_readable_len");
			return m_tail - m_head;
		}

		void move_all(cbuf_len_t offset) {
			move_head(offset);
			move_tail(offset);
		}

		void move_head(cbuf_len_t offset) {
			m_head += offset;
			if (m_head < 0 || m_head > Capacity)
				throw std::exception("move_head");
		}

		void move_tail(cbuf_len_t offset) {
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
			if (get_writable_len() < Capacity / 5) {
				auto len = get_len();
				memcpy(m_buf + ReserverdSize, get_head(), len);

				m_head = ReserverdSize;
				m_tail = len + ReserverdSize;
			}
		}

		void reset() {
			m_head = ReserverdSize;
			m_tail = ReserverdSize;
			ZeroMemory(m_buf, Capacity);
		}

		template <typename T>
		void write_head(const T& value) {
			if (m_head < sizeof(T))
				throw std::exception("write");

			move_head(-sizeof(T));
			T* p = reinterpret_cast<T*>(get_head());
			*p = value;
		}

		template <typename T>
		void write(const T& value) {
			if (get_writable_len() < sizeof(T))
				throw std::exception("write");

			T* p = reinterpret_cast<T*>(get_tail());
			*p = value;

			move_tail(sizeof(T));
		}

		template <typename T, typename ... Args>
		void write(const T& value, Args ... args) {
			write(value);
			write(args ...);
		}

		void write_binary(char* data, cbuf_len_t len/*, cbuf_len_t offset*/) {
			//move_tail(offset);
			if (get_writable_len() < len)
				throw std::exception("write");

			memcpy(get_tail(), data, len);
			move_tail(len);
		}

		template <typename T>
		T read() {
			if (get_readable_len() < sizeof(T))
				throw std::exception("read");

			T ret = *reinterpret_cast<T*>(get_head());
			move_head(sizeof(T));
			return ret;
		}

		template <typename T>
		T get(cbuf_len_t offset = 0) {
			if (offset < m_head || offset + sizeof(T) > m_tail)
				throw std::exception("get");

			return *reinterpret_cast<T*>(get_head() + offset);;
		}

	private:
		cbuf_len_t m_head;
		cbuf_len_t m_tail;

		// use array to avoid heap allocation
		char m_buf[Capacity + ReserverdSize];
	};

}
