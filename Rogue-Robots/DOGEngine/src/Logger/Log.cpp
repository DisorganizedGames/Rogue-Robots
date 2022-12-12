#include "Log.h"

using namespace DOG;

Log::Log() : m_logfile(""), m_rowCount(0)
{}

Log::~Log()
{
	std::ofstream file;
	file.open(m_logfile);

	// write headers to file
	size_t h = 0;
	while (h < m_columns.size())
	{
		file << m_columns[h].Header();
		if (++h < m_columns.size())
			file << ",";
	}
	file << "\n";

	// write rows to file
	for (size_t i = 0; i < m_rowCount; ++i)
	{
		size_t j = 0;
		while (j < m_columns.size())
		{
			file << m_columns[j][i];
			if (++j < m_columns.size())
				file << ",";
		}
		file << "\n";
	}

	file.close();
}

Log& Log::SetLogFile(std::string filename)
{
	m_logfile = filename;
	return *this;
}

Log& Log::DefineColumns(std::vector<std::string> headers)
{
	for (std::string h : headers)
	{
		m_columns.emplace_back(Column(h));
	}
	return *this;
}

Log& Log::NewRow()
{
	++m_rowCount;
	for (Column& col : m_columns)
	{
		col.NewRow();
	}
	return *this;
}
