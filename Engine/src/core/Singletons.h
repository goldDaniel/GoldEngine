#pragma once


#include "Asserts.h"

#include <memory>
#include <functional>
#include <any>
#include <typeindex>

class Singletons
{
public: 
	static Singletons* Get();

	template<typename T>
	using SingletonGenerator = std::function<std::shared_ptr<T>(void)>;


	template<typename T> 
	void Register(SingletonGenerator<T> generator)
	{
		DEBUG_ASSERT(mSingletonMap.find(typeid(T)) == mSingletonMap.end(), "Singleton already registered for type!");
		mSingletonMap[typeid(T)] = { EntryState::Generator, generator };
	}

	template<typename T>
	std::shared_ptr<T> Resolve()
	{
		return _Resolve<T, SingletonGenerator<T>>();
	}

	template<typename T>
	void Erase()
	{
		DEBUG_ASSERT(mSingletonMap.find(typeid(T)) != mSingletonMap.end(), "Singleton not registered for type!");
		mSingletonMap.erase(typeid(T));
	}

private:
	Singletons() {}

	enum class EntryState
	{
		Generator,
		Instance,
		Invalid,
	};

	struct MapEntry
	{
		EntryState mState = EntryState::Invalid;
		std::any mValue{};
	};

	std::unordered_map<std::type_index, MapEntry> mSingletonMap;

	template<class T, class Generator>
	std::shared_ptr<T> _Resolve()
	{
		auto iter = mSingletonMap.find(typeid(T));
		DEBUG_ASSERT(iter != mSingletonMap.end(), "Singleton not registered for type!");

		auto& mapEntry = iter->second;


		switch (mapEntry.mState)
		{
			case EntryState::Generator:
			{
				auto genFunc = std::any_cast<Generator>(mapEntry.mValue);
				mapEntry.mState = EntryState::Instance;
				mapEntry.mValue = genFunc();

				return std::any_cast<std::shared_ptr<T>>(mapEntry.mValue);
			}
			case EntryState::Instance:
			{
				return std::any_cast<std::shared_ptr<T>>(mapEntry.mValue);
			}
			default:
			{
				return nullptr;
			}
		}
	}
};