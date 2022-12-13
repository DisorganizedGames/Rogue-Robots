#pragma once
#include "Column.h"

namespace DOG
{
	class Log
	{
	public:
		Log& CreateLogFile(std::string filename);
		Column& operator[](std::string column);

		Log();
		~Log();

	private:
		std::string m_logfile;
		std::vector<std::string> m_headers;
		std::unordered_map<std::string, Column> m_columns;
	};
}
