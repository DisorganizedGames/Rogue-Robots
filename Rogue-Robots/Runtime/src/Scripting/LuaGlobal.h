#pragma once
#include "LuaTable.h"

class LuaGlobal
{
private:
	LuaW* m_luaW;

public:
	LuaGlobal(LuaW* luaW);
	
	void SetNumber(const std::string& luaGlobalName, int number);
	void SetNumber(const std::string& luaGlobalName, float number);
	void SetNumber(const std::string& luaGlobalName, double number);
	void SetString(const std::string& luaGlobalName, const std::string& text);
	void SetBoolean(const std::string& luaGlobalName, bool boolean);
	template<void (*func)(LuaContext*)>
	void SetFunction(const std::string& luaGlobalName);
	template<typename T>
	void SetUserData(T* object, const std::string& objectName, const std::string& interfaceName);
	void SetLuaInterface(RegisterClassFunctions& registerInterface);

	LuaTable CreateLuaTable(const std::string& luaGlobalName);
	RegisterClassFunctions CreateLuaInterface(const std::string& interfaceName);

	int GetInteger(const std::string& luaGlobalName);
	float GetFloat(const std::string& luaGlobalName);
	double GetDouble(const std::string& luaGlobalName);
	std::string GetString(const std::string& luaGlobalName);
	bool GetBoolean(const std::string& luaGlobalName);
	LuaTable GetTable(const std::string& luaGlobalName);
	template <typename T>
	T* GetUserData(const std::string& luaGlobalName);
};

template<void(*func)(LuaContext*)>
inline void LuaGlobal::SetFunction(const std::string& luaGlobalName)
{
	m_luaW->PushGlobalFunction<func>(luaGlobalName);
}

template<typename T>
inline void LuaGlobal::SetUserData(T* object, const std::string& objectName, const std::string& interfaceName)
{
	m_luaW->PushGlobalUserData<T>(object, interfaceName, objectName);
}

template<typename T>
inline T* LuaGlobal::GetUserData(const std::string& luaGlobalName)
{
	UserData userData = m_luaW->GetGlobalUserData(luaGlobalName);
	T* object = m_luaW->GetUserDataPointer<T>(userData);
	m_luaW->RemoveReferenceToUserData(userData);
	return object;
}
