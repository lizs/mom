// lizs 2017.6.14

#ifndef MOM_LOGGER_H
#define MOM_LOGGER_H

#include "defines.h"
#include "singleton.h"
#include  <spdlog/logger.h>

#define LOG_TRACE(msg, ...)          VK::Logger::instance().trace(msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...)          VK::Logger::instance().debug(msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)           VK::Logger::instance().info(msg, ##__VA_ARGS__) 
#define LOG_WARN(msg, ...)           VK::Logger::instance().warn(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...)          VK::Logger::instance().error(msg, ##__VA_ARGS__)

namespace VK {
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

	private:
		template <typename... Args>
		static void log(std::shared_ptr<spdlog::logger> logger, LogLevel level, const char* fmt, const Args&... args) {
			switch (level) {
				case LogLevel::trace:
					logger->trace(fmt, args...);
					break;
				case LogLevel::debug:
					logger->debug(fmt, args...);
					break;
				case LogLevel::info:
					logger->info(fmt, args...);
					break;
				case LogLevel::warn:
					logger->warn(fmt, args...);
					break;
				case err:
					logger->error(fmt, args...);
					break;
				case critical:
					logger->critical(fmt, args...);
					break;
				case off:
				default: break;
			}
		}

		template <typename... Args>
		void log(LogLevel level, const char* fmt, const Args&... args) {
			if (m_console) {
				log(m_console, level, fmt, args...);
			}
			else {
				PRINT(fmt, args...);
			}

			if (m_daily) {
				log(m_daily, level, fmt, args...);
			}
		}

	public:
		bool start(const char* path, LogLevel level = LogLevel::trace);
		static void stop();

		template <typename... Args>
		void trace(const char* fmt, const Args&... args) {
			log(LogLevel::trace, fmt, args...);
		}

		template <typename... Args>
		void debug(const char* fmt, const Args&... args) {
			log(LogLevel::debug, fmt, args...);
		}

		template <typename... Args>
		void info(const char* fmt, const Args&... args) {
			log(LogLevel::info, fmt, args...);
		}

		template <typename... Args>
		void warn(const char* fmt, const Args&... args) {
			log(LogLevel::warn, fmt, args...);
		}

		template <typename... Args>
		void error(const char* fmt, const Args&... args) {
			log(LogLevel::err, fmt, args...);
		}
	};
}

#endif
