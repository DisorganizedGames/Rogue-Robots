#pragma once
#include "FileWatch/FileWatch.hpp"
#include "LuaTable.h"


struct TempScript
{
	std::string scriptName;
	Table luaScript;
	//LuaTable luaScript;
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

#ifdef _DEBUG
	const std::string c_pathToScripts = "../../../../RunTime/src/Scripting/LuaScripts/";
#else
	const std::string c_pathToScripts = "LuaScripts/";
#endif // _DEBUG

private:
	static void ScriptFileWatcher(const std::filesystem::path& path, const filewatch::Event changeType);
	void TempReloadFile(const std::string& fileName, TempScript* script);

public:
	ScriptManager(LuaW* luaW);
	TempScript* AddScript(const std::string& luaFileName);
	void ReloadScripts();
};