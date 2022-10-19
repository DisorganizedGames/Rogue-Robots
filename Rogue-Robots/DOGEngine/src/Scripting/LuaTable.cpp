#include "LuaTable.h"

namespace DOG
{
	Table& LuaTable::GetTable()
	{
		return m_table;
	}

	//Calls a function on the table
	void LuaTable::CallFunctionOnTableNoReturn(Function function)
	{
		m_luaW->CallTableLuaFunction(m_table, function);
	}

	//Calls a function on the table with the name of the function being required
	void LuaTable::CallFunctionOnTableNoReturn(const std::string& name)
	{
		Function function = m_luaW->GetFunctionFromTable(m_table, name);
		m_luaW->CallTableLuaFunction(m_table, function);
		m_luaW->RemoveReferenceToFunction(function);
	}

	LuaTable::LuaTable()
	{
		m_luaW = &LuaW::GetLuaW();
		m_table = m_luaW->CreateTable();
	}

	LuaTable::LuaTable(Table table, bool addReference) : m_table(table)
	{
		m_luaW = &LuaW::GetLuaW();
		if (addReference)
			m_luaW->AddReferenceToTable(table);
	}


	LuaTable::~LuaTable()
	{
		m_luaW->RemoveReferenceToTable(m_table);
	}

	//Copies the table and adds a reference to it 
	LuaTable::LuaTable(const LuaTable& table)
	{
		m_luaW = table.m_luaW;
		m_table = table.m_table;
		m_luaW->AddReferenceToTable(m_table);
	}

	//Copies the table and adds a reference to it 
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

	//Creates an environment
	void LuaTable::CreateEnvironment(const std::string& luaFileName)
	{
		m_luaW->CreateEnvironment(m_table, luaFileName);
	}

	//Tries creating an environment to check for issues (only checks codes outside of functions)
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

	void LuaTable::AddTableToTable(const std::string& name, LuaTable table)
	{
		Table tableRef = table.GetTable();
		m_luaW->AddTableToTable(m_table, name, tableRef);
	}

	void LuaTable::AddFunctionToTable(const std::string& name, Function& function)
	{
		m_luaW->AddFunctionToTable(m_table, name, function);
	}

	void LuaTable::AddUserDataToTable(const std::string& name, UserData& userData)
	{
		m_luaW->AddUserDataToTable(m_table, name, userData);
	}

	void LuaTable::AddIntToTable(int index, int value)
	{
		m_luaW->AddNumberToTable(m_table, ++index, value);
	}

	void LuaTable::AddFloatToTable(int index, float value)
	{
		m_luaW->AddNumberToTable(m_table, ++index, value);
	}

	void LuaTable::AddDoubleToTable(int index, double value)
	{
		m_luaW->AddNumberToTable(m_table, ++index, value);
	}

	void LuaTable::AddStringToTable(int index, const std::string& string)
	{
		m_luaW->AddStringToTable(m_table, ++index, string);
	}

	void LuaTable::AddBoolToTable(int index, bool boolean)
	{
		m_luaW->AddBoolToTable(m_table, ++index, boolean);
	}

	void LuaTable::AddTableToTable(int index, LuaTable table)
	{
		m_luaW->AddTableToTable(m_table, ++index, table.GetTable());
	}

	void LuaTable::AddFunctionToTable(int index, Function& function)
	{
		m_luaW->AddTableToTable(m_table, ++index, function);
	}

	void LuaTable::AddUserDataToTable(int index, UserData& userData)
	{
		m_luaW->AddUserDataToTable(m_table, ++index, userData);
	}

	LuaTable LuaTable::CreateTableInTable(const std::string& name)
	{
		Table table = m_luaW->CreateTable();
		LuaTable luaTable(table);
		m_luaW->AddTableToTable(m_table, name, table);
		return luaTable;
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

	LuaTable LuaTable::GetTableFromTable(const std::string& name)
	{
		Table table = m_luaW->GetTableFromTable(m_table, name);
		return LuaTable(table);
	}

	UserData LuaTable::GetUserDataFromTable(const std::string& name)
	{
		return m_luaW->GetUserDataFromTable(m_table, name);
	}

	int LuaTable::GetIntFromTable(int index)
	{
		return m_luaW->GetIntegerFromTable(m_table, ++index);
	}

	float LuaTable::GetFloatFromTable(int index)
	{
		return m_luaW->GetFloatFromTable(m_table, ++index);
	}

	double LuaTable::GetDoubleFromTable(int index)
	{
		return m_luaW->GetDoubleFromTable(m_table, ++index);
	}

	std::string LuaTable::GetStringFromTable(int index)
	{
		return std::move(m_luaW->GetStringFromTable(m_table, ++index));
	}

	bool LuaTable::GetBoolFromTable(int index)
	{
		return m_luaW->GetBoolFromTable(m_table, ++index);
	}

	LuaTable LuaTable::GetTableFromTable(int index)
	{
		Table table = m_luaW->GetTableFromTable(m_table, ++index);
		return LuaTable(table);
	}

	UserData LuaTable::GetUserDataFromTable(int index)
	{
		return m_luaW->GetUserDataFromTable(m_table, ++index);
	}

	//Will assert in debug and in release will return an invalid function if the function does not exist
	Function LuaTable::GetFunctionFromTable(const std::string& name)
	{
		if (m_table.ref == -1)
			return { -1 };
		return m_luaW->GetFunctionFromTable(m_table, name);
	}

	//Checks if the function exists and if it does then we will return a valid function reference
	Function LuaTable::TryGetFunctionFromTable(const std::string& name)
	{
		return m_luaW->TryGetFunctionFromTable(m_table, name);
	}

	LuaFunctionReturn LuaTable::CallFunctionOnTable(Function function)
	{
		return m_luaW->CallTableLuaFunctionReturn(m_table, function);
	}

	LuaFunctionReturn LuaTable::CallFunctionOnTable(const std::string& name)
	{
		//Get function
		Function function = m_luaW->GetFunctionFromTable(m_table, name);
		auto luaReturns = m_luaW->CallTableLuaFunctionReturn(m_table, function);
		m_luaW->RemoveReferenceToFunction(function);
		return luaReturns;
	}

	//Will remove the reference to the table on the c++ side which means the lua garbage collection will remove it if there is no reference on the lua side to the table
	void LuaTable::RemoveReferenceToTable()
	{
		m_luaW->RemoveReferenceToTable(m_table);
	}
}