#include "Logger.h"

using namespace DOG;

Logger Logger::s_instance;

Log& Logger::operator[](std::string log)
{
	return m_logs[log];
}
