// Minimal Logger implementation for btop-osd.
//
// The btop collection code logs through the Logger interface; for the OSD we
// simply forward everything to stderr (best-effort, no file rotation, no
// privilege dropping). Logging is disabled unless set_log_level() is called.

#include "btop_log.hpp"

#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>

namespace Logger {
	namespace {
		std::mutex log_mutex;
		Level log_level = Level::WARNING;
	}

	const std::vector<std::string> log_levels = { "DISABLED", "ERROR", "WARNING", "INFO", "DEBUG" };

	void init(const std::filesystem::path& /*path*/) {}

	void set_log_level(Level level) {
		std::lock_guard lock(log_mutex);
		log_level = level;
	}

	void set_log_level(const std::string_view level) {
		for (std::size_t i = 0; i < log_levels.size(); ++i) {
			if (log_levels[i] == level) {
				set_log_level(static_cast<Level>(i));
				return;
			}
		}
	}

	namespace detail {
		[[nodiscard]] auto is_enabled(Level level) -> bool {
			std::lock_guard lock(log_mutex);
			return log_level >= level && log_level != Level::DISABLED;
		}

		void log_write(Level level, const std::string_view msg) {
			std::lock_guard lock(log_mutex);
			if (!(log_level >= level && log_level != Level::DISABLED)) return;
			const auto name = log_levels[static_cast<std::uint8_t>(level)];
			std::fprintf(stderr, "[%s] %.*s\n", name.c_str(), static_cast<int>(msg.size()), msg.data());
		}
	} // namespace detail
} // namespace Logger
