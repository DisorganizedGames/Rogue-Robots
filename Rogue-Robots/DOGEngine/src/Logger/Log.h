#pragma once
#include "Column.h"

namespace DOG
{
	class Log
	{
	public:
		void SaveLogFile(std::string filename);
		Column& operator[](std::string column);

		Log()  = default;
		~Log() = default;

	private:
		std::vector<std::string> m_headers;
		std::unordered_map<std::string, Column> m_columns;
	};
}
