#pragma once
#include "Column.h"

namespace DOG
{
	class Log
	{
		friend class Logger;

	public:
		Column& operator[](std::string column);

		Log()  = default;
		~Log() = default;

	private:
		void SaveLogFile(std::string filename);

		std::vector<std::string> m_headers;
		std::unordered_map<std::string, Column> m_columns;
	};
}
