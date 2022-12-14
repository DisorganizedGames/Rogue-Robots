#include "Logger.h"
#include <sstream>
#include <cstring>

using namespace DOG;

Logger::Logger(const std::filesystem::path& path, const std::filesystem::path& preserve) noexcept : m_filePath(path)
{
	if (preserve.string().length() == 0) 
		return;
	
	std::ifstream file(preserve);
	if (!file)
	{
		std::cerr << "Non Fatal: Preserve specified, but no file to preserve\n";
		return;
	}

	std::string line;

	std::getline(file, line);
	auto it = line.begin();
	while (it != line.end() && line.length() != 0)
	{
		auto end = std::find(it, line.end(), ',');
		m_columns.emplace_back(it, end);
		it = end == line.end() ? end : end + 1;
	}

	m_values.resize(m_columns.size());

	while (std::getline(file, line))
	{
		auto it = line.begin();
		u64 index = 0;
		while (it != line.end() && line.length() != 0)
		{
			auto end = std::find(it, line.end(), ',');
			m_values[index++].emplace_back(it, end);
			it = end == line.end() ? end : end + 1;
		}
	}
}

Logger::~Logger()
{
	std::ofstream file(m_filePath);
	std::stringstream ss;
	for (auto& col: m_columns)
	{
		ss << col << ",";
	}
	auto str = ss.str();
	file.write(str.c_str(), str.size()-1); // Don't write the final comma
	file << "\n";
	
	u64 index = 0;
	while (true)
	{
		ss = std::stringstream();
		bool anyLeft = false;
		for (auto& col: m_values) 
		{
			if (index >= col.size())
			{
				ss << ",";
				continue;
			}

			anyLeft = true;
			ss << col[index] << ",";
		}
		if (!anyLeft) 
			break;

		str = ss.str();
		file.write(str.c_str(), str.size()-1); // Don't write the final comma
		file << "\n";

		index++;
	}
}

u64 Logger::AddColumn(const std::string& name) noexcept
{
	u64 out = m_columns.size();
	m_columns.emplace_back(name);
	m_values.resize(m_columns.size());
	return out;
}

