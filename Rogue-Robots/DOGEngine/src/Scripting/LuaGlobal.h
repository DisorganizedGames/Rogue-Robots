#pragma once
#include "LuaTable.h"

namespace DOG
{
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
		T* GetUserDataPointer(const std::string& luaGlobalName);
		UserData GetUserData(const std::string& luaGlobalName);

		template <class... Args>
		LuaFunctionReturn CallGlobalFunction(Function& function, Args... args);
		template <class... Args>
		LuaFunctionReturn CallGlobalFunction(const std::string& name, Args... args);
		LuaFunctionReturn CallGlobalFunction(Function function);
		LuaFunctionReturn CallGlobalFunction(const std::string& name);
	};

	//Sets the given name to a global function
	template<void(*func)(LuaContext*)>
	inline void LuaGlobal::SetFunction(const std::string& luaGlobalName)
	{
		m_luaW->PushGlobalFunction<func>(luaGlobalName);
	}

	//Sets the given name to a global userData
	template<typename T>
	inline void LuaGlobal::SetUserData(T* object, const std::string& objectName, const std::string& interfaceName)
	{
		m_luaW->PushGlobalUserData<T>(object, interfaceName, objectName);
	}

	//Get the global userData
	template<typename T>
	inline T* LuaGlobal::GetUserDataPointer(const std::string& luaGlobalName)
	{
		UserData userData = m_luaW->GetGlobalUserData(luaGlobalName);
		T* object = m_luaW->GetUserDataPointer<T>(userData);
		m_luaW->RemoveReferenceToUserData(userData);
		return object;
	}

	template<class ...Args>
	inline LuaFunctionReturn LuaGlobal::CallGlobalFunction(Function& function, Args ...args)
	{
		//Get the amount of arguments
		int argumentSize = sizeof...(Args);
		//Push the arguments to the stack
		m_luaW->PushStack(args...);

		return m_luaW->CallLuaFunctionReturn(function, argumentSize);
	}

	template<class ...Args>
	inline LuaFunctionReturn LuaGlobal::CallGlobalFunction(const std::string& name, Args ...args)
	{
		//Get function
		Function function = m_luaW->GetGlobalFunction(name);
		//Get the amount of arguments
		int argumentSize = sizeof...(Args);
		//Push the arguments to the stack
		m_luaW->PushStack(args...);

		auto luaReturns = m_luaW->CallLuaFunctionReturn(function, argumentSize);
		m_luaW->RemoveReferenceToFunction(function);
		return luaReturns;
	}
}