#pragma once
#include <fstream>
#include <filesystem>

namespace DOG
{		
	class Logger
	{
		using u64 = unsigned long long;

	private:
		std::filesystem::path m_filePath;
		std::vector<std::string> m_columns;
		std::vector<std::vector<std::string>> m_values;

	public:
		Logger() = default;
		Logger(const std::filesystem::path& path, const std::filesystem::path& preserve="") noexcept;
		~Logger();

		u64 AddColumn(const std::string& name) noexcept;

		template<class ValueClass>
		void PushValue(const u64 column, const ValueClass value) noexcept;
	};

	template<class ValueClass>
	void Logger::PushValue(const u64 column, const ValueClass value) noexcept
	{
		m_values[column].push_back(std::to_string(value));
	}
}

