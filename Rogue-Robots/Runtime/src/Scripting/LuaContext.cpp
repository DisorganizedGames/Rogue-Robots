#include "LuaContext.h"

void LuaContext::IncreaseReturnArgument()
{
	//Clears the stack if it's not empty
	if (m_returnArgumentSize == 0)
	{
		m_luaW->ClearStack();
	}
	++m_returnArgumentSize;
}

LuaContext::LuaContext(LuaW* luaW)
{
	m_luaW = luaW;
	m_returnArgumentSize = 0;
}

//if there exist no integer then we return zero
int LuaContext::GetInteger() const
{
	const int max = m_luaW->GetNumberOfStackItems();

	for (int index = 1; index <= max; ++index)
		if (m_luaW->IsInteger(index))
			return m_luaW->GetIntegerFromStack(index);

	std::cout << "Error: Couldn't Find Integer In Arguments\n";

	return 0;
}

//if there exist no double/float then we return zero
double LuaContext::GetDouble() const
{
	const int max = m_luaW->GetNumberOfStackItems();

	for (int index = 1; index <= max; ++index)
		if (m_luaW->IsNumber(index) && !m_luaW->IsInteger(index))
			return m_luaW->GetDoubleFromStack(index);

	std::cout << "Error: Couldn't Find Float/Double In Arguments\n";

	return 0.0;
}

//if there exist no bool then we return false
bool LuaContext::GetBoolean() const
{
	const int max = m_luaW->GetNumberOfStackItems();

	for (int index = 1; index <= max; ++index)
		if (m_luaW->IsBool(index))
			return m_luaW->GetBoolFromStack(index);

	std::cout << "Error: Couldn't Find Boolean In Arguments\n";

	return false;
}

//if there exist no string then we return a empty string
std::string LuaContext::GetString() const
{
	const int max = m_luaW->GetNumberOfStackItems();

	for (int index = 1; index <= max; ++index)
		if (!m_luaW->IsNumber(index) && m_luaW->IsString(index))
			return m_luaW->GetStringFromStack(index);

	std::cout << "Error: Couldn't Find String In Arguments\n";

	return std::string();
}

//if there exist no table it returns a empty table
LuaTable LuaContext::GetTable() const
{
	const int max = m_luaW->GetNumberOfStackItems();

	for (int index = 1; index <= max; ++index)
		if (m_luaW->IsTable(index))
		{
			Table getTable = m_luaW->GetTableFromStack(index);
			LuaTable table(m_luaW, getTable);
			return table;
		}

	std::cout << "Error: Couldn't Find Table In Arguments\n";

	return LuaTable(m_luaW);
}

void LuaContext::ReturnInteger(int integer)
{
	IncreaseReturnArgument();
	m_luaW->PushIntegerToStack(integer);
}

void LuaContext::ReturnDouble(double number)
{
	IncreaseReturnArgument();
	m_luaW->PushDoubleToStack(number);
}

void LuaContext::ReturnBoolean(bool boolean)
{
	IncreaseReturnArgument();
	m_luaW->PushBoolToStack(boolean);
}

void LuaContext::ReturnString(const std::string& string)
{
	IncreaseReturnArgument();
	m_luaW->PushStringToStack(string);
}

void LuaContext::ReturnTable(LuaTable& luaTable)
{
	IncreaseReturnArgument();
	m_luaW->PushTableToStack(luaTable.GetTable());
}

uint32_t LuaContext::GetNumberOfReturns() const
{
	return m_returnArgumentSize;
}
