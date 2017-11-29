// lizs 2017.6.14

#ifndef MOM_LOGGER_H
#define MOM_LOGGER_H

#include "singleton.h"
#include  <spdlog/logger.h>

#define LOG_TRACE(msg, ...)          VK::Logger::instance().trace(msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...)          VK::Logger::instance().debug(msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)           VK::Logger::instance().info(msg, ##__VA_ARGS__) 
#define LOG_WARN(msg, ...)           VK::Logger::instance().warn(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...)          VK::Logger::instance().error(msg, ##__VA_ARGS__)

namespace VK {
#define _LOG_(level, fmt, ...) \
	if (m_console){	\
		m_console->#level(fmt, args...);	\
	}else{	\
		PRINT(fmt, args...);	\
	}	\
	if	(m_daily)	\
		m_daily->#level(fmt, args...);	\


	enum LogLevel {
		trace = 0,
		debug = 1,
		info = 2,
		warn = 3,
		err = 4,
		critical = 5,
		off = 6
	};

	//	singleton of logger
	class NET_EXPORT Logger {
		SINGLETON(Logger)
		std::shared_ptr<spdlog::logger> m_console;
		std::shared_ptr<spdlog::logger> m_daily;

	public:
		bool start(const char* path, LogLevel level = LogLevel::trace);
		static void stop();

		template <typename... Args>
		void trace(const char* fmt, const Args&... args) {
			//_LOG_(trace, fmt, args...);
		}

		template <typename... Args>
		void debug(const char* fmt, const Args&... args) {
			//_LOG_(debug, fmt, args...);
		}

		template <typename... Args>
		void info(const char* fmt, const Args&... args) {
			//_LOG_(info, fmt, args...);
		}

		template <typename... Args>
		void warn(const char* fmt, const Args&... args) {
			//_LOG_(warn, fmt, args...);
		}

		template <typename... Args>
		void error(const char* fmt, const Args&... args) {
			//_LOG_(error, fmt, args...);
		}
	};
}

#endif
