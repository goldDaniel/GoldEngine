#pragma once 

#include "Core.h"
#include <spdlog/spdlog.h>

namespace gold
{
	using LogMessage = spdlog::details::log_msg;

	class Logging
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger> EngineLogger()
		{
			return kEngineLogger;
		}

		inline static std::shared_ptr<spdlog::logger> ClientLogger()
		{
			return kClientLogger;
		}

		static void AddCallback(std::function<void(const LogMessage& msg)>&& callback);

		static void ClearLog();

		static const std::vector<std::pair<int, std::string>>& GetLogHistory();

	private:
		static std::vector<std::pair<int, std::string>> kLogHistory;

		static std::shared_ptr<spdlog::logger> kEngineLogger;
		static std::shared_ptr<spdlog::logger> kClientLogger;
	};
}



#if defined(GOLD_ENGINE)

#define G_ENGINE_FATAL(...) ::gold::Logging::EngineLogger()->critical(__VA_ARGS__)
#define G_ENGINE_ERROR(...) ::gold::Logging::EngineLogger()->error(__VA_ARGS__)
#define G_ENGINE_WARN(...)  ::gold::Logging::EngineLogger()->warn(__VA_ARGS__)
#define G_ENGINE_INFO(...)  ::gold::Logging::EngineLogger()->info(__VA_ARGS__)
#define G_ENGINE_TRACE(...) ::gold::Logging::EngineLogger()->trace(__VA_ARGS__)

#endif

#define G_FATAL(...) ::gold::Logging::ClientLogger()->critical(__VA_ARGS__)
#define G_ERROR(...) ::gold::Logging::ClientLogger()->error(__VA_ARGS__)
#define G_WARN(...)  ::gold::Logging::ClientLogger()->warn(__VA_ARGS__)
#define G_INFO(...)  ::gold::Logging::ClientLogger()->info(__VA_ARGS__)
#define G_TRACE(...) ::gold::Logging::ClientLogger()->trace(__VA_ARGS__)


