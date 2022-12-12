#pragma once
#include "Column.h"

namespace DOG
{
	class Log
	{
	public:
		Log& SetLogFile(std::string filename);
		Log& DefineColumns(std::vector<std::string> headers);
		Column& operator[](size_t columnID) { return m_columns[columnID]; }
		Log& NewRow();

		Log();
		~Log();

	private:
		std::string m_logfile;
		size_t m_rowCount;
		std::vector<Column> m_columns;
	};
}
