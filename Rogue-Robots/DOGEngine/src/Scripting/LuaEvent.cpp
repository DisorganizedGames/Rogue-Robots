#include "LuaEvent.h"
#include "ScriptManager.h"
#include "LuaGlobal.h"

namespace DOG
{
	LuaEvent::LuaEvent(LuaW* luaW, ScriptManager* scriptManager) : m_luaW(luaW)
	{
		const std::string& luaFileEventSystemName = "EventSystem.lua";
		const std::string& luaEventSystemTableName = "EventSystem";

		scriptManager->RunLuaFile(luaFileEventSystemName);

		Table table = m_luaW->GetGlobalTable(luaEventSystemTableName);
		m_eventSystemTable = std::make_unique<LuaTable>(table);

		m_eventSystemTableRegisterFunction = m_eventSystemTable->GetFunctionFromTable("Register");
		m_eventSystemTableInvokeFunction = m_eventSystemTable->GetFunctionFromTable("InvokeEvent");
	}

	void LuaEvent::InvokeEvent(const std::string& eventName)
	{
		//Calls the EventSystem on Lua
		m_eventSystemTable->CallFunctionOnTable(m_eventSystemTableInvokeFunction, eventName);
	}
}