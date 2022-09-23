#include "LuaMain.h"

namespace DOG
{
	LuaMain LuaMain::s_luaMain;

	LuaMain::LuaMain()
	{
	}

	LuaMain::~LuaMain()
	{
	}

	void LuaMain::Initialize()
	{
		s_luaMain.m_global = std::make_unique<LuaGlobal>(&LuaW::GetLuaW());
		s_luaMain.m_scriptManager = std::make_unique<ScriptManager>(&LuaW::GetLuaW());
		s_luaMain.m_event = std::make_unique<LuaEvent>(&LuaW::GetLuaW(), s_luaMain.m_scriptManager.get());
	}

	LuaGlobal* LuaMain::GetGlobal()
	{
		return s_luaMain.m_global.get();
	}

	LuaEvent* LuaMain::GetEventSystem()
	{
		return s_luaMain.m_event.get();
	}
	ScriptManager* LuaMain::GetScriptManager()
	{
		return s_luaMain.m_scriptManager.get();
	}
}