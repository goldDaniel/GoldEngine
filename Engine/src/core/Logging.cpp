#include "Logging.h"

#include "Core.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/callback_sink.h"

#include <sstream>
#include <iomanip>

using namespace gold;

std::shared_ptr<spdlog::logger> Logging::kEngineLogger;
std::shared_ptr<spdlog::logger> Logging::kClientLogger;

std::vector<std::pair<int, std::string>> Logging::kLogHistory;


void Logging::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");


	kEngineLogger = spdlog::stdout_color_mt("ENGINE");
	kEngineLogger->set_level(spdlog::level::trace);

	kClientLogger = spdlog::stdout_color_mt("APP");
	kClientLogger->set_level(spdlog::level::trace);

	Logging::AddCallback([](const LogMessage& msg)
		{
			std::time_t time = std::chrono::system_clock::to_time_t(msg.time);

			// [TIME] LOGGER: message
			std::ostringstream oss;
			oss << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] ";
			oss << msg.logger_name.data() << ": ";

			std::string message = oss.str() + std::string(msg.payload.data(), msg.payload.size());
			kLogHistory.push_back({ msg.level, std::move(message) });
		});
}

void Logging::AddCallback(std::function<void(const spdlog::details::log_msg& msg)>&& callback)
{
	auto logCallback = std::make_shared<spdlog::sinks::callback_sink_mt>(std::move(callback));

#if defined(GOLD_ENGINE)
	kEngineLogger->sinks().push_back(logCallback);
#else
	kClientLogger->sinks().push_back(logCallback);
#endif
}

void Logging::ClearLog()
{
	kLogHistory.clear();
}

const std::vector<std::pair<int, std::string>>& Logging::GetLogHistory()
{
	return kLogHistory;
}