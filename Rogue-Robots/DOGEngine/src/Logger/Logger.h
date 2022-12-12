#pragma once
#include "Log.h"

namespace DOG
{
	class Logger
	{
	public:
		[[nodiscard]] static constexpr Logger& Get() noexcept { return s_instance; }
		Log& operator[](std::string log);

	private:
		static Logger s_instance;
		std::unordered_map<std::string, Log> m_logs;

		Logger() = default;
		~Logger() = default;
	};
}