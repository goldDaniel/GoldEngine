#include "Logging.h"

#include "Core.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/callback_sink.h"

#include <sstream>
#include <iomanip>

using namespace gold;


Logging::Logging()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");


	mEngineLogger = spdlog::stdout_color_mt("ENGINE");
	mEngineLogger->set_level(spdlog::level::trace);

	mClientLogger = spdlog::stdout_color_mt("APP");
	mClientLogger->set_level(spdlog::level::trace);

	AddCallback([this](const LogMessage& msg)
	{
		std::time_t time = std::chrono::system_clock::to_time_t(msg.time);

		// [TIME] LOGGER: message
		std::ostringstream oss;
		oss << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] ";
		oss << msg.logger_name.data() << ": ";

		std::string message = oss.str() + std::string(msg.payload.data(), msg.payload.size());
		mLogHistory.push_back({ msg.level, std::move(message) });
	});
}

void Logging::AddCallback(std::function<void(const spdlog::details::log_msg& msg)>&& callback)
{
	auto logCallback = std::make_shared<spdlog::sinks::callback_sink_mt>(std::move(callback));

	mEngineLogger->sinks().push_back(logCallback);
	mClientLogger->sinks().push_back(logCallback);
}

void Logging::ClearLog()
{
	mLogHistory.clear();
}

const std::vector<std::pair<int, std::string>>& Logging::GetLogHistory()
{
	return mLogHistory;
}