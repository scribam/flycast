#include "Log.h"

#ifdef LIBRETRO
#include "libretro_sink.h"
#else
#include "cfg/cfg.h"
#include "stdclass.h"
#endif

#define LOGGER_USE_LEGACY_FORMAT

#include <spdlog/spdlog.h>
#ifdef LOGGER_USE_LEGACY_FORMAT
#include <spdlog/details/fmt_helper.h>
#include <spdlog/pattern_formatter.h>
#endif

#ifndef LIBRETRO
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#ifdef USE_BREAKPAD
#include <spdlog/sinks/ringbuffer_sink.h>
#endif

#include <algorithm>
#include <array>
#include <locale>
#include <memory>
#include <string>
#include <vector>

#ifdef LOGGER_USE_LEGACY_FORMAT
class custom_path_formatter_flag final : public spdlog::custom_flag_formatter
{
public:
	static std::size_t DeterminePathCutOffPoint(const char *filename)
	{
		constexpr const char *pattern = "/core/";
#ifdef _WIN32
		constexpr const char *pattern2 = "\\core\\";
#endif
		std::string path = filename;
		std::transform(path.begin(), path.end(), path.begin(),
					   [](char c) { return std::tolower(c, std::locale::classic()); });
		std::size_t pos = path.find(pattern);
#ifdef _WIN32
		if (pos == std::string::npos)
			pos = path.find(pattern2);
#endif
		if (pos != std::string::npos)
			return pos + strlen(pattern);
		return 0;
	}

	void format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest) override
	{
		static auto m_path_cutoff_point = DeterminePathCutOffPoint(msg.source.filename);
		spdlog::details::fmt_helper::append_string_view(msg.source.filename + m_path_cutoff_point, dest);
		dest.push_back(':');
		spdlog::details::fmt_helper::append_int(msg.source.line, dest);
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return spdlog::details::make_unique<custom_path_formatter_flag>();
	}
};

#ifndef LIBRETRO
class custom_time_formatter_flag final : public spdlog::custom_flag_formatter
{
public:
	void format(const spdlog::details::log_msg &, const std::tm &, spdlog::memory_buf_t &dest) override
	{
		u64 now = getTimeMs();
		u32 minutes = (u32) now / 60;
		u32 seconds = (u32) now % 60;
		u32 ms = (now - (u32) now) * 1000;

		spdlog::details::fmt_helper::pad2(minutes, dest);
		dest.push_back(':');
		spdlog::details::fmt_helper::pad2(seconds, dest);
		dest.push_back(':');
		spdlog::details::fmt_helper::pad3(ms, dest);
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return spdlog::details::make_unique<custom_time_formatter_flag>();
	}
};
#endif
#endif

namespace LogManager
{
	// TODO: consistency between LogManager::Type and this array 
	constexpr std::array names = {
		"AICA",
		"AICA_ARM",
		"AUDIO",
		"BOOT",
		"COMMON",
		"DYNAREC",
		"FLASHROM",
		"GDROM",
		"HOLLY",
		"INPUT",
		"INTERPRETER",
		"JVS",
		"MAPLE",
		"MEMORY",
		"MODEM",
		"NAOMI",
		"NETWORK",
		"PROFILER",
		"PVR",
		"REIOS",
		"RENDERER",
		"SAVESTATE",
		"SH4",
		"VMEM"
	};
	
	void Init([[maybe_unused]] void *log_cb)
	{
		std::vector<spdlog::sink_ptr> sinks;

#ifdef USE_BREAKPAD
		sinks.push_back(std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(20));
#endif

#ifdef LIBRETRO
		sinks.push_back(std::make_shared<libretro_sink_mt>((retro_log_printf_t)log_cb));
#else
		if (cfgLoadBool("log", "LogToConsole", true)) {
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		}

		if (cfgLoadBool("log", "LogToFile", false)) {
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("flycast.log", true));
		}
#endif

		for (const auto &name: names) {
#ifdef LIBRETRO
			spdlog::level::level_enum level = spdlog::level::trace;
#else
			spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(cfgLoadInt("log", name, spdlog::level::info));
#endif

			auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
			logger->set_level(level);
			spdlog::register_logger(logger);
		}

#ifdef LOGGER_USE_LEGACY_FORMAT
		auto formatter = spdlog::details::make_unique<spdlog::pattern_formatter>();
		formatter->add_flag<custom_path_formatter_flag>('j');
#ifdef LIBRETRO
		formatter->set_pattern("%j %L[%n]: %v");
#else
		formatter->add_flag<custom_time_formatter_flag>('k');
		formatter->set_pattern("%k %j %L[%n]: %v");
#endif
		spdlog::set_formatter(std::move(formatter));
#endif
	}

	void Shutdown()
	{
		spdlog::shutdown();
	}

	std::shared_ptr<spdlog::logger> GetLoggerByType(LogManager::Type type)
	{
		return spdlog::get(names[type]);
	}
}
