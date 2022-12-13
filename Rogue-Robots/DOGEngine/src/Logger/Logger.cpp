#include "Logger.h"

using namespace DOG;

Logger Logger::s_instance;

Logger::~Logger()
{
	for (auto& [name, log] : m_logs)
		log.SaveLogFile(name + ".csv");
}

Log& Logger::operator[](std::string log)
{
	return m_logs[log];
}
