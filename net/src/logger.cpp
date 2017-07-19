// lizs 2017.6.14
#include "logger.h"
#include "str.h"
#include "file.h"
#include <spdlog/spdlog.h>

namespace VK {
	const char * LOGGER_ROOT = "log";
	bool Logger::start(const char* path) {
		// 确保路径存在
		auto items = std::move(split(const_cast<char*>(path), "/\\"));
		if (items.empty()) {
			return false;
		}

		if (items[0] != LOGGER_ROOT) {
			items.insert(items.begin(), LOGGER_ROOT);
		}

		auto name = items[items.size() - 1];
		items.erase(items.end() - 1);

		auto dir = std::move(join(items, '/'));
		if (!make_dir(dir))
			return false;

		// 日志
		try {
			spdlog::set_pattern("[%T] [%t] [%l] %v");
			spdlog::set_level(spdlog::level::trace);

			// console
			m_console = spdlog::stdout_color_mt("console");

			// file
			spdlog::set_async_mode(128);
			m_daily = spdlog::daily_logger_mt("async_file_logger", dir + "/" + name);
			m_daily->flush_on(spdlog::level::trace);

			return true;
		}
		catch (...) {
			return false;
		}
	}

void VK::Logger::stop() {
	spdlog::drop_all();
}

}
