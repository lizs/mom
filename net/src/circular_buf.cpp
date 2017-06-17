#include "circular_buf.h"
#include "_singleton_.h"
#include "bytes_pool.h"
#include "monitor.h"
#include "logger.h"

namespace VK {
	namespace Net {
		CircularBuf::CircularBuf(): m_capacity(0),
		                            m_reserved(0),
		                            m_head(0),
		                            m_tail(0),
		                            m_buf(nullptr) {
#if MONITOR_ENABLED
			_singleton_<Monitor>::instance().inc_pcb_count();
#endif 
		}

		CircularBuf::~CircularBuf() {
			clear();
#if MONITOR_ENABLED
			_singleton_<Monitor>::instance().dec_pcb_count();
#endif
		}

		void CircularBuf::clear() {
			if (m_buf != nullptr) {
				_singleton_<BytesPool>::instance().dealloc(m_buf, m_capacity);
				m_buf = nullptr;
			}
		}

		cbuf_len_t CircularBuf::get_len() const {
			ASSERT(m_tail >= m_head);
			return m_tail - m_head;
		}

		cbuf_len_t CircularBuf::get_writable_len() const {
			///ASSERT(m_capacity > m_tail);
			return m_capacity - m_tail;
		}

		bool CircularBuf::move_head(cbuf_offset_t offset) {
			m_head += offset;
			if (m_head < 0 || m_head > m_capacity) {
				Logger::instance().error("move_head failed, m_head : {} offset : {}", m_head, offset);
				return false;
			}
			return true;
		}

		bool CircularBuf::move_tail(cbuf_offset_t offset) {
			m_tail += offset;
			if (m_tail < 0 || m_tail > m_capacity) {
				Logger::instance().error("move_tail failed, m_tail : {} offset : {}", m_tail, offset);
				return false;
			}
			return true;
		}

		char* CircularBuf::get_head_ptr() const {
			return m_buf + m_head;
		}

		char* CircularBuf::get_tail_ptr() const {
			return m_buf + m_tail;
		}

		void CircularBuf::set_head(cbuf_len_t pos) {
			m_head = pos;
		}

		void CircularBuf::set_tail(cbuf_len_t pos) {
			m_tail = pos;
		}

		cbuf_len_t CircularBuf::get_head() const {
			return m_head;
		}

		cbuf_len_t CircularBuf::get_tail() const {
			return m_tail;
		}

		void CircularBuf::arrange() {
			if (m_capacity - m_tail < m_capacity / 5) {
				auto len = get_len();
				memcpy(m_buf + m_reserved, get_head_ptr(), len);

				m_head = m_reserved;
				m_tail = len + m_reserved;
			}
		}

		void CircularBuf::reset(cbuf_len_t size, cbuf_len_t reserved_size) {
			clear();

			m_reserved = reserved_size;
			m_capacity = reserved_size + size;
			m_buf = _singleton_<BytesPool>::instance().alloc(m_capacity);
			reuse();
		}

		void CircularBuf::reuse() {
			m_head = m_reserved;
			m_tail = m_reserved;
			ZeroMemory(m_buf, m_capacity);
		}

		bool CircularBuf::write(const char* str) {
			strncpy(get_head_ptr(), str, strnlen(str, m_capacity - m_reserved - 1));
			return write<byte_t>(0);
		}

		bool CircularBuf::write_binary(char* data, cbuf_len_t len) {
			if (get_writable_len() < len) {
				Logger::instance().error("write_binary failed, writable_len : {} < len : {}", get_writable_len(), len);
				return false;
			}

			memcpy(get_tail_ptr(), data, len);
			return move_tail(len);
		}

		cbuf_ptr_t alloc_cbuf(cbuf_len_t len) {
			cbuf_ptr_t pcb(_singleton_<cbuf_pool_t>::instance().alloc());
			pcb->reset(len);

			return pcb;
		}
	}
}
