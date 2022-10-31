#pragma once
#include "LuaTable.h"

namespace DOG
{
	class LuaContext;
	class ScriptManager;

	class LuaEvent
	{
	private:
		LuaW* m_luaW;

		std::unique_ptr<LuaTable> m_eventSystemTable;
		Function m_eventSystemTableRegisterFunction;
		Function m_eventSystemTableInvokeFunction;

	public:
		LuaEvent(LuaW* luaW, ScriptManager* scriptManager);
		template <void(*func)(LuaContext*)>
		void Register(const std::string& eventName);
		void InvokeEvent(const std::string& eventName);
		template<class ...Args>
		void InvokeEvent(const std::string& eventName, Args&&... args);
		void RemoveEvent(const std::string& eventName);
	};

	template<void(*func)(LuaContext*)>
	inline void LuaEvent::Register(const std::string& eventName)
	{
		Function callBack = m_luaW->HookFunctionAndGetFunction<func>();
		//Pushes the callBack function to the EventSystem
		m_eventSystemTable->CallFunctionOnTable(m_eventSystemTableRegisterFunction, eventName, callBack);
	}

	template<class ...Args>
	inline void LuaEvent::InvokeEvent(const std::string& eventName, Args&& ...args)
	{
		//Calls the EventSystem on Lua
		m_eventSystemTable->CallFunctionOnTable(m_eventSystemTableInvokeFunction, eventName, std::forward<Args>(args)...);
	}
}