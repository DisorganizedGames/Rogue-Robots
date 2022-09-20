#pragma once
#include "LuaTable.h"

class LuaContext;
class ScriptManager;

class LuaEvent
{
private:
	LuaW* m_luaW;
	static LuaEvent s_luaEvent;
	//std::unordered_map<std::string, std::vector<std::function<void (void*)>>> eventCallBacks;
	std::unique_ptr<LuaTable> m_eventSystemTable;
	Function m_eventSystemTableRegisterFunction;
	Function m_eventSystemTableInvokeFunction;

private:
	LuaEvent();

public:
	void Initialize(LuaW* luaW, ScriptManager* scriptManager);
	static LuaEvent& GetLuaEvent();

	template<void (*func)(LuaContext*)>
	void AddListener(const std::string& eventName);

	//template <typename Func, class... Args>
	//void AddListener(const std::string& eventName, Func callBack, Args&& ...args);
	//template <typename Func>
	//void AddListener(const std::string& eventName, Func callBack);
	//template <void (*func)()>
	//void AddListenerT(const std::string& eventName);
	/*void FireEvent(const std::string& eventName, ...);*/
};

template<void (*func)(LuaContext*)>
inline void LuaEvent::AddListener(const std::string& eventName)
{
	//m_eventSystemTable->CallFunctionOnTable();
	//m_eventSystemTable->Add
	//m_luaW.Push
}