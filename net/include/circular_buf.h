// author : lizs
// 2017.2.21
#pragma once
#include <exception>
#include "net.h"

namespace VK {
	namespace Net {
		template <cbuf_len_t Capacity>
		class CircularBuf final {
			const cbuf_len_t ReserverdSize = 32;
		public:
			explicit CircularBuf()
				: m_head(ReserverdSize),
				m_tail(ReserverdSize) { }

			cbuf_len_t get_len() const {
				if (m_tail < m_head) throw std::exception("get_len");
				return m_tail - m_head;
			}

			cbuf_len_t get_writable_len() const {
				if (Capacity < m_tail) throw std::exception("get_writable_len");
				return Capacity - m_tail;
			}

			bool move_head(cbuf_offset_t offset) {
				m_head += offset;
				if (m_head < 0 || m_head > Capacity) {
					LOG("move_head failed, m_head : %d offset : %d", m_head, offset);
					return false;
				}
				return true;
			}

			bool move_tail(cbuf_offset_t offset) {
				m_tail += offset;
				if (m_tail < 0 || m_tail > Capacity) {
					LOG("move_tail failed, m_tail : %d offset : %d", m_tail, offset);
					return false;
				}
				return true;
			}

			template <typename T>
			bool move_tail() {
				return move_tail(sizeof(T));
			}

			char* get_head_ptr() {
				return m_buf + m_head;
			}

			template <typename T>
			T* get_head_ptr() {
				return reinterpret_cast<T*>(m_buf + m_head);
			}

			char* get_tail_ptr() {
				return m_buf + m_tail;
			}

			void set_head(cbuf_len_t pos) {
				m_head = pos;
			}

			void set_tail(cbuf_len_t pos) {
				m_tail = pos;
			}

			cbuf_len_t get_head() const {
				return m_head;
			}

			cbuf_len_t get_tail() const {
				return m_tail;
			}

			void arrange() {
				if (get_writable_len() < Capacity / 5) {
					auto len = get_len();
					memcpy(m_buf + ReserverdSize, get_head_ptr(), len);

					m_head = ReserverdSize;
					m_tail = len + ReserverdSize;
				}
			}

			void reset() {
				m_head = ReserverdSize;
				m_tail = ReserverdSize;
				ZeroMemory(m_buf, Capacity);
			}

			void reset(cbuf_len_t offset) {
				if (offset > Capacity) {
					return;
				}

				m_head = ReserverdSize;
				m_tail = ReserverdSize;
				ZeroMemory(m_buf + offset, Capacity - offset);
			}

			template <typename T>
			bool write_head(const T& value) {
				if (m_head < sizeof(T))
					return false;

				if (!move_head(-static_cast<cbuf_offset_t>(sizeof(T)))) return false;
				T* p = reinterpret_cast<T*>(get_head_ptr());
				*p = value;
				return true;
			}

			template <typename T, typename ... Args>
			bool write_head(const T& value, Args ... args) {
				return write_head(value) && write_head(args ...);
			}

			template <typename T>
			T * pre_write_head() {
				if (m_head < sizeof(T))
					return nullptr;

				if (!move_head(-static_cast<cbuf_offset_t>(sizeof(T))))
					return nullptr;

				return reinterpret_cast<T*>(get_head_ptr());
			}

			template <typename T>
			T* pre_write() {
				if (get_writable_len() < sizeof(T))
					return nullptr;

				T* p = reinterpret_cast<T*>(get_tail_ptr());
				if (!move_tail(sizeof(T)))
					return nullptr;

				return p;
			}

			template <typename T>
			bool write(const T& value) {
				if (get_writable_len() < sizeof(T))
					return false;

				T* p = reinterpret_cast<T*>(get_tail_ptr());
				*p = value;

				return move_tail(sizeof(T));
			}

			template <typename T, typename ... Args>
			bool write(const T& value, Args ... args) {
				return write(value) && write(args ...);
			}

			bool write_binary(char* data, cbuf_len_t len) {
				if (get_writable_len() < len) {
					LOG("write_binary failed, writable_len : %d < len : %d", get_writable_len(), len);
					return false;
				}

				memcpy(get_tail_ptr(), data, len);
				return move_tail(len);
			}

			template <typename T>
			bool read(T& out) {
				if (get_len() < sizeof(T))
					return false;

				out = *reinterpret_cast<T*>(get_head_ptr());
				return move_head(sizeof(T));
			}

			template <typename T>
			bool get(T& out, cbuf_offset_t offset = 0) {
				if (offset < 0 || m_head + offset + sizeof(T) > m_tail)
					return false;

				out = *reinterpret_cast<T*>(get_head_ptr() + offset);
				return true;
			}

		private:
			cbuf_len_t m_head;
			cbuf_len_t m_tail;

			// use array to avoid heap allocation
			char m_buf[Capacity];
		};
	}
}
