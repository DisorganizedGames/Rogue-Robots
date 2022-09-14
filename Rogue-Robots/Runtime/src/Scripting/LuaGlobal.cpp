#include "LuaGlobal.h"

LuaGlobal::LuaGlobal(LuaW* luaW) : m_luaW (luaW) 
{
}

void LuaGlobal::SetNumber(const std::string& luaGlobalName, int number)
{
	m_luaW->PushGlobalNumber(luaGlobalName, number);
}

void LuaGlobal::SetNumber(const std::string& luaGlobalName, float number)
{
	m_luaW->PushGlobalNumber(luaGlobalName, number);
}

void LuaGlobal::SetNumber(const std::string& luaGlobalName, double number)
{
	m_luaW->PushGlobalNumber(luaGlobalName, number);
}

void LuaGlobal::SetString(const std::string& luaGlobalName, const std::string& text)
{
	m_luaW->PushGlobalString(luaGlobalName, text);
}

void LuaGlobal::SetBoolean(const std::string& luaGlobalName, bool boolean)
{
	m_luaW->PushGlobalBool(luaGlobalName, boolean);
}

void LuaGlobal::SetLuaInterface(RegisterClassFunctions& registerInterface)
{
	m_luaW->PushLuaInterface(registerInterface);
}

LuaTable LuaGlobal::CreateLuaTable(const std::string& luaGlobalName)
{
	Table table = m_luaW->CreateGlobalTable(luaGlobalName);
	return LuaTable(m_luaW, table);
}

RegisterClassFunctions LuaGlobal::CreateLuaInterface(const std::string& interfaceName)
{
	return m_luaW->CreateLuaInterface(interfaceName);
}

int LuaGlobal::GetInteger(const std::string& luaGlobalName)
{
	return m_luaW->GetGlobalInteger(luaGlobalName);
}

float LuaGlobal::GetFloat(const std::string& luaGlobalName)
{
	return m_luaW->GetGlobalFloat(luaGlobalName);
}

double LuaGlobal::GetDouble(const std::string& luaGlobalName)
{
	return m_luaW->GetGlobalDouble(luaGlobalName);
}

std::string LuaGlobal::GetString(const std::string& luaGlobalName)
{
	return m_luaW->GetGlobalString(luaGlobalName);
}

bool LuaGlobal::GetBoolean(const std::string& luaGlobalName)
{
	return m_luaW->GetGlobalBool(luaGlobalName);;
}

LuaTable LuaGlobal::GetTable(const std::string& luaGlobalName)
{
	Table table = m_luaW->GetGlobalTable(luaGlobalName);
	return LuaTable(m_luaW, table);
}
