// lizs 2017.6.14
#pragma once
#include "_singleton_.h"
#include  <spdlog/logger.h>

namespace VK {
	//	��־����
	class NET_EXPORT Logger : public _singleton_<Logger> {
		std::shared_ptr<spdlog::logger> m_console;
		std::shared_ptr<spdlog::logger> m_daily;

	public:
		bool start(const char* path);
		static void stop();
		
		template <typename... Args>
		void trace(const char* fmt, const Args&... args) {
			m_console->trace(fmt, args...);
			m_daily->trace(fmt, args...);
		}

		template <typename... Args>
		void debug(const char* fmt, const Args&... args) {
			m_console->debug(fmt, args...);
			m_daily->debug(fmt, args...);
		}

		template <typename... Args>
		void info(const char* fmt, const Args&... args) {
			m_console->info(fmt, args...);
			m_daily->info(fmt, args...);
		}

		template <typename... Args>
		void warn(const char* fmt, const Args&... args) {
			m_console->warn(fmt, args...);
			m_daily->warn(fmt, args...);
		}

		template <typename... Args>
		void error(const char* fmt, const Args&... args) {
			m_console->error(fmt, args...);
			m_daily->error(fmt, args...);
		}
	};
}
