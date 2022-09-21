#pragma once
#include "LuaW.h"

class LuaContext;

class LuaTable
{
	friend LuaContext;

private:
	LuaW* m_luaW;
	Table m_table;

private:
	Table& GetTable();
	void CallFunctionOnTableNoReturn(Function function);
	void CallFunctionOnTableNoReturn(const std::string& name);

public:
	LuaTable(LuaW* luaW);
	LuaTable(LuaW* luaW, Table& table, bool addReference = false);
	~LuaTable();
	LuaTable(const LuaTable& table);
	LuaTable& operator=(const LuaTable& table);

	void CreateEnvironment(const std::string& luaFileName);
	bool TryCreateEnvironment(const std::string& luaFileName);

	void AddIntToTable(const std::string& name, int value);
	void AddFloatToTable(const std::string& name, float value);
	void AddDoubleToTable(const std::string& name, double value);
	void AddStringToTable(const std::string& name, const std::string& string);
	void AddBoolToTable(const std::string& name, bool boolean);
	void AddTableToTable(const std::string& name, LuaTable table);
	void AddFunctionToTable(const std::string& name, Function& function);
	template <void (*func)(LuaContext*)>
	Function AddFunctionToTable(const std::string& name);
	template <typename T>
	UserData AddUserDataToTable(const std::string& name, T* object, const std::string& interfaceName);
	void AddUserDataToTable(const std::string& name, UserData& userData);

	LuaTable CreateTableInTable(const std::string& name);

	int GetIntFromTable(const std::string& name);
	float GetFloatFromTable(const std::string& name);
	double GetDoubleFromTable(const std::string& name);
	std::string GetStringFromTable(const std::string& name);
	bool GetBoolFromTable(const std::string& name);
	LuaTable GetTableFromTable(const std::string& name);
	//Get Pointer from the UserData from Lua table
	template <typename T>
	T* GetUserDataPointerFromTable(const std::string& name);
	//Get UserData from Lua table
	UserData GetUserDataFromTable(const std::string& name);

	Function GetFunctionFromTable(const std::string& name);
	Function TryGetFunctionFromTable(const std::string& name);
	template <class... Args>
	LuaFunctionReturn CallFunctionOnTable(Function function, Args... args);
	template <class... Args>
	LuaFunctionReturn CallFunctionOnTable(const std::string& name, Args... args);
	LuaFunctionReturn CallFunctionOnTable(Function function);
	LuaFunctionReturn CallFunctionOnTable(const std::string& name);

	void RemoveReferenceToTable();
};

template<void(*func)(LuaContext*)>
inline Function LuaTable::AddFunctionToTable(const std::string& name)
{
	m_luaW->AddFunctionToTable<func>(m_table, name); 
	return GetFunctionFromTable(name);
}

template<typename T>
inline UserData LuaTable::AddUserDataToTable(const std::string& name, T* object, const std::string& interfaceName)
{
	m_luaW->AddUserDataToTable(m_table, object, interfaceName, name);
	return GetUserDataFromTable(name);
}

template<typename T>
inline T* LuaTable::GetUserDataPointerFromTable(const std::string& name)
{
	return m_luaW->GetUserDataPointer<T>(m_luaW->GetUserDataFromTable());
}

template<class ...Args>
inline LuaFunctionReturn LuaTable::CallFunctionOnTable(Function function, Args ...args)
{
	//Get the amount of arguments
	int argumentSize = sizeof...(Args);
	//Push the arguments to the stack
	m_luaW->PushStack(args...);

	return m_luaW->CallTableLuaFunctionReturn(m_table, function, argumentSize);
}

template<class ...Args>
inline LuaFunctionReturn LuaTable::CallFunctionOnTable(const std::string& name, Args ...args)
{
	//Get function
	Function function = m_luaW->GetFunctionFromTable(m_table, name);
	//Get the amount of arguments
	int argumentSize = sizeof...(Args);
	//Push the arguments to the stack
	m_luaW->PushStack(args...);

	auto luaReturns = m_luaW->CallTableLuaFunctionReturn(m_table, function, argumentSize);
	m_luaW->RemoveReferenceToFunction(function);
	return luaReturns;
}
