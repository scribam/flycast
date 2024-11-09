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
	
    void Init(void* log_cb = nullptr);
    void Shutdown();
	std::shared_ptr<spdlog::logger> GetLoggerByType(LogManager::Type type);
}

#define ERROR_LOG(t, ...) \
    SPDLOG_LOGGER_ERROR(GetLoggerByType(LogManager::Type::t), fmt::sprintf(__VA_ARGS__))

#define WARN_LOG(t, ...) \
    SPDLOG_LOGGER_WARN(GetLoggerByType(LogManager::Type::t), fmt::sprintf(__VA_ARGS__))

#define NOTICE_LOG(t, ...) \
    SPDLOG_LOGGER_INFO(GetLoggerByType(LogManager::Type::t), fmt::sprintf(__VA_ARGS__))

#define INFO_LOG(t, ...) \
    SPDLOG_LOGGER_INFO(GetLoggerByType(LogManager::Type::t), fmt::sprintf(__VA_ARGS__))

#define DEBUG_LOG(t, ...) \
    SPDLOG_LOGGER_DEBUG(GetLoggerByType(LogManager::Type::t), fmt::sprintf(__VA_ARGS__))
