#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/printf.h>

#include <memory>

namespace LogManager
{
	enum Type
	{
		AICA,
		AICA_ARM,
		AUDIO,
		BOOT,
		COMMON,
		DYNAREC,
		FLASHROM,
		GDROM,
		HOLLY,
		INPUT,
		INTERPRETER,
		JVS,
		MAPLE,
		MEMORY,
		MODEM,
		NAOMI,
		NETWORK,
		PROFILER,
		PVR,
		REIOS,
		RENDERER,
		SAVESTATE,
		SH4,
		VMEM
	};
	static constexpr uint32_t NUMBER_OF_LOGGERS = 24;

	struct LogContainer
	{
		const std::string short_name;
		const std::string long_name;
		std::shared_ptr<spdlog::logger> logger;
	};
	
	void Init(void* log_cb = nullptr);
	void Shutdown();
	LogContainer GetLogContainer(Type type);
}

#define ERROR_LOG(t, ...) \
	SPDLOG_LOGGER_ERROR(GetLogContainer(LogManager::Type::t).logger, fmt::sprintf(__VA_ARGS__))

#define WARN_LOG(t, ...) \
	SPDLOG_LOGGER_WARN(GetLogContainer(LogManager::Type::t).logger, fmt::sprintf(__VA_ARGS__))

#define NOTICE_LOG(t, ...) \
	SPDLOG_LOGGER_INFO(GetLogContainer(LogManager::Type::t).logger, fmt::sprintf(__VA_ARGS__))

#define INFO_LOG(t, ...) \
	SPDLOG_LOGGER_INFO(GetLogContainer(LogManager::Type::t).logger, fmt::sprintf(__VA_ARGS__))

#define DEBUG_LOG(t, ...) \
	SPDLOG_LOGGER_DEBUG(GetLogContainer(LogManager::Type::t).logger, fmt::sprintf(__VA_ARGS__))
