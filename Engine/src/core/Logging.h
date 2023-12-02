#pragma once 

// TODO (danielg): P1, this causes really long compile times. Templated lib included as header only
#include <spdlog/spdlog.h>

namespace gold
{
	using LogMessage = spdlog::details::log_msg;

	class Logging
	{
	public:
		Logging();

		inline std::shared_ptr<spdlog::logger> EngineLogger()
		{
			return mEngineLogger;
		}

		inline std::shared_ptr<spdlog::logger> ClientLogger()
		{
			return mClientLogger;
		} 

		void AddCallback(std::function<void(const LogMessage& msg)>&& callback);

		void ClearLog();

		const std::vector<std::pair<int, std::string>>& GetLogHistory();

	private:
		std::vector<std::pair<int, std::string>> mLogHistory;

		std::shared_ptr<spdlog::logger> mEngineLogger;
		std::shared_ptr<spdlog::logger> mClientLogger;
	};
}

#if defined(GOLD_ENGINE)

#define G_ENGINE_FATAL(...) Singletons::Get()->Resolve<gold::Logging>()->EngineLogger()->critical(__VA_ARGS__)
#define G_ENGINE_ERROR(...) Singletons::Get()->Resolve<gold::Logging>()->EngineLogger()->error(__VA_ARGS__)
#define G_ENGINE_WARN(...)  Singletons::Get()->Resolve<gold::Logging>()->EngineLogger()->warn(__VA_ARGS__)
#define G_ENGINE_INFO(...)  Singletons::Get()->Resolve<gold::Logging>()->EngineLogger()->info(__VA_ARGS__)
#define G_ENGINE_TRACE(...) Singletons::Get()->Resolve<gold::Logging>()->EngineLogger()->trace(__VA_ARGS__)

#endif

#define G_FATAL(...) Singletons::Get()->Resolve<gold::Logging>()->ClientLogger()->critical(__VA_ARGS__)
#define G_ERROR(...) Singletons::Get()->Resolve<gold::Logging>()->ClientLogger()->error(__VA_ARGS__)
#define G_WARN(...)  Singletons::Get()->Resolve<gold::Logging>()->ClientLogger()->warn(__VA_ARGS__)
#define G_INFO(...)  Singletons::Get()->Resolve<gold::Logging>()->ClientLogger()->info(__VA_ARGS__)
#define G_TRACE(...) Singletons::Get()->Resolve<gold::Logging>()->ClientLogger()->trace(__VA_ARGS__)


