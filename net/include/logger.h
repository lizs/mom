// lizs 2017.6.14
#pragma once
#include "_singleton_.h"
#include  <spdlog/logger.h>

namespace VK {
#define _LOG_(level, fmt, ...) \
	if (m_console){	\
		m_console->##level(fmt, args...);	\
	}else{	\
		PRINT(fmt, args...);	\
	}	\
	if	(m_daily)	\
		m_daily->##level(fmt, args...);	\


	//	ÈÕÖ¾µ¥Àý
	class NET_EXPORT Logger : public _singleton_<Logger> {
		std::shared_ptr<spdlog::logger> m_console;
		std::shared_ptr<spdlog::logger> m_daily;

	public:
		bool start(const char* path);
		static void stop();

		template <typename... Args>
		void trace(const char* fmt, const Args&... args) {
			_LOG_(trace, fmt, args...);
		}

		template <typename... Args>
		void debug(const char* fmt, const Args&... args) {
			_LOG_(debug, fmt, args...);
		}

		template <typename... Args>
		void info(const char* fmt, const Args&... args) {
			_LOG_(info, fmt, args...);
		}

		template <typename... Args>
		void warn(const char* fmt, const Args&... args) {
			_LOG_(warn, fmt, args...);
		}

		template <typename... Args>
		void error(const char* fmt, const Args&... args) {
			_LOG_(error, fmt, args...);
		}
	};
}
