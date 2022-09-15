#pragma once
#pragma warning(push, 0)
#include "FileWatch/FileWatch.hpp"
#pragma warning(pop)
#include "LuaTable.h"


struct TempScript
{
	std::string scriptName;
	Table luaScript;
	Function onStart;
	Function onUpdate;
};

class ScriptManager
{
private:
	std::unordered_map<std::string, std::vector<TempScript>> m_scriptsMap;
	LuaW* m_luaW;
	std::unique_ptr<filewatch::FileWatch<std::filesystem::path>> m_fileWatcher;
	static std::vector<std::string> s_filesToBeReloaded;
	static std::mutex s_reloadMutex;
	const std::string c_pathToScripts = "Assets/LuaScripts/";

private:
	static void ScriptFileWatcher(const std::filesystem::path& path, const filewatch::Event changeType);
	//Temp before component system
	void TempReloadFile(const std::string& fileName, TempScript* script);

public:
	ScriptManager(LuaW* luaW);
	//Temp before component system
	TempScript* AddScript(const std::string& luaFileName);
	void ReloadScripts();
};