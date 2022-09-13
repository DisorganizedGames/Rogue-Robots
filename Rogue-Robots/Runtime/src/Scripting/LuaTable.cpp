#include "LuaTable.h"

Table& LuaTable::GetTable()
{
	return m_table;
}

LuaTable::LuaTable(LuaW* luaW) : m_luaW(luaW)
{
	m_table = luaW->CreateTable();
}

LuaTable::LuaTable(LuaW* luaW, Table& table) : m_luaW(luaW), m_table(table)
{
}

LuaTable::~LuaTable()
{
	m_luaW->RemoveReferenceToTable(m_table);
}

LuaTable::LuaTable(const LuaTable& table)
{
	m_luaW = table.m_luaW;
	m_table = table.m_table;
	m_luaW->AddReferenceToTable(m_table);
}

LuaTable& LuaTable::operator=(const LuaTable& table)
{
	if (this == &table)
		return *this;

	m_luaW->RemoveReferenceToTable(m_table);

	m_luaW = table.m_luaW;
	m_table = table.m_table;

	m_luaW->AddReferenceToTable(m_table);

	return *this;
}

void LuaTable::CreateEnvironment(const std::string& luaFileName)
{
	m_luaW->CreateEnvironment(m_table, luaFileName);
}

bool LuaTable::TryCreateEnvironment(const std::string& luaFileName)
{
	return m_luaW->TryCreateEnvironment(m_table, luaFileName);
}

void LuaTable::AddIntToTable(const std::string& name, int value)
{
	m_luaW->AddNumberToTable(m_table, name, value);
}

void LuaTable::AddFloatToTable(const std::string& name, float value)
{
	m_luaW->AddNumberToTable(m_table, name, value);
}

void LuaTable::AddDoubleToTable(const std::string& name, double value)
{
	m_luaW->AddNumberToTable(m_table, name, value);
}

void LuaTable::AddStringToTable(const std::string& name, const std::string& string)
{
	m_luaW->AddStringToTable(m_table, name, string);
}

void LuaTable::AddBoolToTable(const std::string& name, bool boolean)
{
	m_luaW->AddBoolToTable(m_table, name, boolean);
}

int LuaTable::GetIntFromTable(const std::string& name)
{
	return m_luaW->GetIntegerFromTable(m_table, name);
}

float LuaTable::GetFloatFromTable(const std::string& name)
{
	return m_luaW->GetFloatFromTable(m_table, name);
}

double LuaTable::GetDoubleFromTable(const std::string& name)
{
	return m_luaW->GetDoubleFromTable(m_table, name);
}

std::string LuaTable::GetStringFromTable(const std::string& name)
{
	return std::move(m_luaW->GetStringFromTable(m_table, name));
}

bool LuaTable::GetBoolFromTable(const std::string& name)
{
	return m_luaW->GetBoolFromTable(m_table, name);
}

Function LuaTable::GetFunctionFromTable(const std::string& name)
{
	return m_luaW->GetFunctionFromTable(m_table, name);
}

Function LuaTable::TryGetFunctionFromTable(const std::string& name)
{
	return m_luaW->TryGetFunctionFromTable(m_table, name);
}

void LuaTable::CallFunctionOnTable(Function function)
{
	m_luaW->CallTableLuaFunction(m_table, function);
}

void LuaTable::CallFunctionOnTable(const std::string& name)
{
	Function function = m_luaW->GetFunctionFromTable(m_table, name);
	m_luaW->CallTableLuaFunction(m_table, function);
	m_luaW->RemoveReferenceToFunction(function);
}

//Will remove the reference to the table on the c++ side which means the lua garbage collection will remove it if there is no reference on the lua side to the table
void LuaTable::RemoveReferenceToTable()
{
	m_luaW->RemoveReferenceToTable(m_table);
}