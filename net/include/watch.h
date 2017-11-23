// 2017/3/11

#ifndef MOM_WATCH_H
#define MOM_WATCH_H

#include <chrono>
#include "defines.h"

namespace VK {
	// ��ʱ��
	class NET_EXPORT Watch {
		typedef std::chrono::duration<int, std::milli> milliseconds_t;
		typedef std::chrono::duration<int> seconds_t;
		typedef std::chrono::high_resolution_clock clock;
	public:
		void start() {
			m_begin = clock::now();
		}

		void stop() {
			m_end = clock::now();
		}

		long long elapsed_milli_seconds() const {
			return std::chrono::duration_cast<milliseconds_t>(m_end - m_begin).count();
		}

		long long elapsed_seconds() const {
			return std::chrono::duration_cast<seconds_t>(m_end - m_begin).count();
		}

	private:
		clock::time_point m_begin;
		clock::time_point m_end;
	};
}

#endif