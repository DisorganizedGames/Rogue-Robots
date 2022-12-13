#include "Log.h"

using namespace DOG;

Column& Log::operator[](std::string column)
{ 
	Column& col = m_columns[column];
	if (m_headers.size() < m_columns.size())
		m_headers.push_back(column);
	return col;
}

void Log::SaveLogFile(std::string filename)
{
	std::ofstream file;
	file.open(filename);

	// write headers to file
	size_t h = 0;
	size_t rowCount = 0;
	while (h < m_headers.size())
	{
		rowCount = std::max(rowCount, m_columns[m_headers[h]].size());
		file << m_headers[h];
		if (++h < m_headers.size())
			file << ",";
	}
	file << "\n";

	// write rows to file
	for (size_t i = 0; i < rowCount; ++i)
	{
		size_t j = 0;
		while (j < m_columns.size())
		{
			auto& header = m_headers[j];
			if (j < m_columns[header].size())
				file << m_columns[header][i];
			else
				file << "";
			if (++j < m_columns.size())
				file << ",";
		}
		file << "\n";
	}

	file.close();
}
