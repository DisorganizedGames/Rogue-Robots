#include "LuaGlobal.h"

namespace DOG
{
	LuaGlobal::LuaGlobal(LuaW* luaW) : m_luaW(luaW)
	{
	}

	//Sets the given name to a global number
	void LuaGlobal::SetNumber(const std::string& luaGlobalName, int number)
	{
		m_luaW->PushGlobalNumber(luaGlobalName, number);
	}

	//Sets the given name to a global number
	void LuaGlobal::SetNumber(const std::string& luaGlobalName, float number)
	{
		m_luaW->PushGlobalNumber(luaGlobalName, number);
	}

	//Sets the given name to a global number
	void LuaGlobal::SetNumber(const std::string& luaGlobalName, double number)
	{
		m_luaW->PushGlobalNumber(luaGlobalName, number);
	}

	//Sets the given name to a global string
	void LuaGlobal::SetString(const std::string& luaGlobalName, const std::string& text)
	{
		m_luaW->PushGlobalString(luaGlobalName, text);
	}

	//Sets the given name to a global bool
	void LuaGlobal::SetBoolean(const std::string& luaGlobalName, bool boolean)
	{
		m_luaW->PushGlobalBool(luaGlobalName, boolean);
	}

	//Creates a lua interface to access c++ functions and member functions
	void LuaGlobal::SetLuaInterface(RegisterClassFunctions& registerInterface)
	{
		m_luaW->PushLuaInterface(registerInterface);
	}

	//Creates a new table and returns it
	LuaTable LuaGlobal::CreateLuaTable(const std::string& luaGlobalName)
	{
		Table table = m_luaW->CreateGlobalTable(luaGlobalName);
		return LuaTable(table);
	}

	//Returns a object which the user uses to create the interface
	RegisterClassFunctions LuaGlobal::CreateLuaInterface(const std::string& interfaceName)
	{
		return m_luaW->CreateLuaInterface(interfaceName);
	}

	//Get the global integer
	int LuaGlobal::GetInteger(const std::string& luaGlobalName)
	{
		return m_luaW->GetGlobalInteger(luaGlobalName);
	}

	//Get the global float
	float LuaGlobal::GetFloat(const std::string& luaGlobalName)
	{
		return m_luaW->GetGlobalFloat(luaGlobalName);
	}

	//Get the global double
	double LuaGlobal::GetDouble(const std::string& luaGlobalName)
	{
		return m_luaW->GetGlobalDouble(luaGlobalName);
	}

	//Get the global string
	std::string LuaGlobal::GetString(const std::string& luaGlobalName)
	{
		return m_luaW->GetGlobalString(luaGlobalName);
	}

	//Get the global bool
	bool LuaGlobal::GetBoolean(const std::string& luaGlobalName)
	{
		return m_luaW->GetGlobalBool(luaGlobalName);;
	}

	//Get the global table
	LuaTable LuaGlobal::GetTable(const std::string& luaGlobalName)
	{
		Table table = m_luaW->GetGlobalTable(luaGlobalName);
		return LuaTable(table);
	}

	//Get the global UserData
	UserData LuaGlobal::GetUserData(const std::string& luaGlobalName)
	{
		return m_luaW->GetGlobalUserData(luaGlobalName);
	}

	LuaFunctionReturn LuaGlobal::CallGlobalFunction(Function& function)
	{
		return m_luaW->CallLuaFunctionReturn(function);
	}

	LuaFunctionReturn LuaGlobal::CallGlobalFunction(const std::string& name)
	{
		//Get function
		Function function = m_luaW->GetGlobalFunction(name);

		auto luaReturns = m_luaW->CallLuaFunctionReturn(function);
		m_luaW->RemoveReferenceToFunction(function);
		return luaReturns;
	}
}