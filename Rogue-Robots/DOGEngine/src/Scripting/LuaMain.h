#pragma once
#include "LuaGlobal.h"
#include "LuaEvent.h"
#include "LuaContext.h"
#include "ScriptManager.h"

namespace DOG
{
	class LuaMain
	{
	private:
		std::unique_ptr<LuaGlobal> m_global;
		std::unique_ptr<ScriptManager> m_scriptManager;
		std::unique_ptr<LuaEvent> m_event;
		static LuaMain s_luaMain;

	private:
		LuaMain();

	public:
		~LuaMain();
		LuaMain(const LuaMain& other) = delete;
		LuaMain& operator=(const LuaMain& other) = delete;

		static void Initialize();

		static LuaGlobal* GetGlobal();
		static LuaEvent* GetEventSystem();
		static ScriptManager* GetScriptManager();
	};
}