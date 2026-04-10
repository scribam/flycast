#include <libretro-common/include/libretro.h>
#include <spdlog/sinks/base_sink.h>

template<typename Mutex>
class libretro_sink : public spdlog::sinks::base_sink<Mutex> {
private:
	retro_log_printf_t retro_log_printf;

public:
	explicit libretro_sink(retro_log_printf_t log_printf) {
		retro_log_printf = log_printf;
	}

protected:
	void sink_it_(const spdlog::details::log_msg &msg) override {
		spdlog::memory_buf_t formatted;
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

		switch (msg.level) {
			case spdlog::level::trace:
			case spdlog::level::debug:
				retro_log_printf(RETRO_LOG_DEBUG, fmt::to_string(formatted).c_str());
				break;
			case spdlog::level::info:
				retro_log_printf(RETRO_LOG_INFO, fmt::to_string(formatted).c_str());
				break;
			case spdlog::level::warn:
				retro_log_printf(RETRO_LOG_WARN, fmt::to_string(formatted).c_str());
				break;
			case spdlog::level::err:
			case spdlog::level::critical:
				retro_log_printf(RETRO_LOG_ERROR, fmt::to_string(formatted).c_str());
				break;
			default:
				break;
		}
	}

	void flush_() override {

	}
};

#include <spdlog/details/null_mutex.h>
#include <mutex>

using libretro_sink_mt = libretro_sink<std::mutex>;
