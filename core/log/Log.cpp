#include "Log.h"

#ifdef LIBRETRO
#include "libretro_sink.h"
#else
#include "cfg/cfg.h"
#include "stdclass.h"
#endif

#include <spdlog/spdlog.h>
#include <spdlog/details/fmt_helper.h>
#include <spdlog/pattern_formatter.h>

#ifndef LIBRETRO
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/udp_sink.h>
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

class custom_path_formatter_flag final : public spdlog::custom_flag_formatter
{
public:
    static std::size_t DeterminePathCutOffPoint(const char* filename)
    {
        constexpr const char* pattern = "/core/";
#ifdef _WIN32
        constexpr const char* pattern2 = "\\core\\";
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

    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
    {
        static auto m_path_cutoff_point = DeterminePathCutOffPoint(msg.source.filename);
        spdlog::details::fmt_helper::append_string_view(msg.source.filename + m_path_cutoff_point, dest);
    }

    [[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return std::make_unique<custom_path_formatter_flag>();
    }
};

#ifndef LIBRETRO
class custom_time_formatter_flag final : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override
    {
        u64 now = getTimeMs();
        u32 ms = (u32)(now % 1000);
        now /= 1000;
        u32 seconds = (u32)(now % 60);
        now /= 60;
        u32 minutes = (u32)now;

        spdlog::details::fmt_helper::pad2(minutes, dest);
        dest.push_back(':');
        spdlog::details::fmt_helper::pad2(seconds, dest);
        dest.push_back(':');
        spdlog::details::fmt_helper::pad3(ms, dest);
    }

    [[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return std::make_unique<custom_time_formatter_flag>();
    }
};
#endif

namespace LogManager
{
    std::array<LogContainer, NUMBER_OF_LOGGERS> logContainers = {
        {
            {"AICA", "AICA Audio Emulation", nullptr},
            {"AICA_ARM", "AICA ARM Emulation", nullptr},
            {"AUDIO", "Audio Ouput Interface", nullptr},
            {"BOOT", "Boot", nullptr},
            {"COMMON", "Common", nullptr},
            {"DYNAREC", "Dynamic Recompiler", nullptr},
            {"FLASHROM", "FlashROM / EEPROM", nullptr},
            {"GDROM", "GD-Rom Drive", nullptr},
            {"HOLLY", "Holly Chipset", nullptr},
            {"INPUT", "Input Peripherals", nullptr},
            {"INTERPRETER", "SH4 Interpreter", nullptr},
            {"JVS", "Naomi JVS Protocol", nullptr},
            {"MAPLE", "Maple Bus and Peripherals", nullptr},
            {"MEMORY", "Memory Management", nullptr},
            {"MODEM", "Modem and Network", nullptr},
            {"NAOMI", "Naomi", nullptr},
            {"NETWORK", "Naomi Network", nullptr},
            {"PROFILER", "Performance Profiler", nullptr},
            {"PVR", "GPU Emulation", nullptr},
            {"REIOS", "HLE BIOS", nullptr},
            {"RENDERER", "Graphics Renderer", nullptr},
            {"SAVESTATE", "Save States", nullptr},
            {"SH4", "SH4 Modules", nullptr},
            {"VMEM", "Virtual Memory Management", nullptr}
        }
    };

    void Init([[maybe_unused]] void* log_cb)
    {
        std::vector<spdlog::sink_ptr> sinks;

#ifdef LIBRETRO
        sinks.push_back(std::make_shared<libretro_sink_mt>((retro_log_printf_t)log_cb));

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<custom_path_formatter_flag>('j');
        formatter->set_pattern("%j:%# %L[%n]: %v");

        for (auto& logContainer : logContainers)
        {
            logContainer.logger = std::make_shared<spdlog::logger>(logContainer.short_name, sinks.begin(), sinks.end());
            logContainer.logger->set_formatter(formatter->clone());
            logContainer.logger->set_level(spdlog::level::debug);
        }
#else
#ifdef USE_BREAKPAD
        sinks.push_back(std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(20));
#endif

        if (config::loadBool("log", "LogToConsole", true))
        {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }

        if (config::loadBool("log", "LogToFile", false))
        {
            sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("flycast.log", true));
        }

        std::string logServer = config::loadStr("log", "LogServer", "");
        if (!logServer.empty())
        {
            std::string host;
            std::uint16_t port = 31667;
            auto colon = logServer.find(':');
            if (colon != std::string::npos)
            {
                host = logServer.substr(0, colon);
                port = static_cast<std::uint16_t>(std::stoi(logServer.substr(colon + 1)));
            }
            else
            {
                host = logServer;
            }
            spdlog::sinks::udp_sink_config udpSinkConfig(host, port);
            sinks.push_back(std::make_shared<spdlog::sinks::udp_sink_mt>(udpSinkConfig));
        }

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<custom_path_formatter_flag>('j');
        formatter->add_flag<custom_time_formatter_flag>('k');
        formatter->set_pattern("%^%k %j:%# %L[%n]: %v%$");

        spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(config::loadInt("log", "Verbosity", spdlog::level::debug));

        for (auto& logContainer : logContainers)
        {
            logContainer.logger = std::make_shared<spdlog::logger>(logContainer.short_name, sinks.begin(), sinks.end());
            logContainer.logger->set_formatter(formatter->clone());

            if (config::loadBool("log", logContainer.short_name, true))
                logContainer.logger->set_level(level);
            else
                logContainer.logger->set_level(spdlog::level::off);
        }
#endif
    }

    void Shutdown()
    {
        for (auto& logContainer : logContainers)
        {
            logContainer.logger.reset();
        }
    }

    LogContainer GetLogContainer(Type type)
    {
        return logContainers[type];
    }
}
