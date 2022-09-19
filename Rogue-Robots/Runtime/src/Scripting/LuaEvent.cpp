#include "LuaEvent.h"
#include "ScriptManager.h"
#include "LuaTable.h"

LuaEvent LuaEvent::s_luaEvent;

void LuaEvent::Initialize(LuaW* luaW, ScriptManager* scriptManager)
{
	const std::string& luaFileEventSystemName = "EventSystem.lua";
	const std::string& luaEventSystemTableName = "EventSystem";

	s_luaEvent.m_luaW = luaW;
	scriptManager->RunLuaFile(luaFileEventSystemName); m_luaW->GetGlobalTable(luaEventSystemTableName);

	Table table = m_luaW->GetGlobalTable(luaEventSystemTableName);
	m_eventSystemTable = std::make_unique<LuaTable>(m_luaW, table);
}

LuaEvent& LuaEvent::GetLuaEvent()
{
	return s_luaEvent;
}

//void LuaEvent::FireEvent(const std::string& eventName, ...)
//{
//	
//}
