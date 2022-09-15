#include "ScriptManager.h"

std::vector<std::string> ScriptManager::s_filesToBeReloaded = {};
std::mutex ScriptManager::s_reloadMutex;


//Check the file that have been modified and make sure it is a lua file and pushes it to be reloaded at a later stage
void ScriptManager::ScriptFileWatcher(const std::filesystem::path& path, const filewatch::Event changeType)
{
	if (filewatch::Event::modified == changeType && path.extension() == std::filesystem::path(".lua"))
	{
		std::string fileName = path.string();

		std::lock_guard<std::mutex> guard(s_reloadMutex);
		s_filesToBeReloaded.push_back(fileName);
	}
}

//Reloades the script
void ScriptManager::TempReloadFile(const std::string& fileName, TempScript* script)
{
	//Will test lua syntax
	bool failedLoadingFile = m_luaW->TryLoadChunk(c_pathToScripts + fileName);

	if (failedLoadingFile)
		return;

	//Will only test code outside of functions
	//Maybe add more
	//Maybe should skip this entirely
	LuaTable testTable(m_luaW);
	bool failedCreatingEnvironment = testTable.TryCreateEnvironment(c_pathToScripts + fileName);

	if (failedCreatingEnvironment)
		return;

	//Removes the old environment and creates a new one
	m_luaW->RemoveReferenceToTable(script->luaScript);
	script->luaScript = m_luaW->CreateTable();
	m_luaW->CreateEnvironment(script->luaScript, c_pathToScripts + fileName);
}

ScriptManager::ScriptManager(LuaW* luaW) : m_luaW(luaW)
{
#ifdef _DEBUG
	//Hook up the script watcher
	std::wstring watchPath = L"./" + std::wstring(c_pathToScripts.begin(), c_pathToScripts.end());
	m_fileWatcher = std::make_unique<filewatch::FileWatch<std::filesystem::path>>(watchPath, [](const std::filesystem::path& path, const filewatch::Event changeType) 
		{
			ScriptManager::ScriptFileWatcher(path, changeType);
		}
	);
#endif // _DEBUG
}

//Creates a script and runs it
TempScript* ScriptManager::AddScript(const std::string& luaFileName)
{
	TempScript newScript = {luaFileName, -1, 0, 0};
	newScript.luaScript = m_luaW->CreateTable();
	
	LuaTable table(m_luaW, newScript.luaScript, true);
	table.CreateEnvironment(c_pathToScripts + luaFileName);
	newScript.onStart = table.TryGetFunctionFromTable("OnStart");
	newScript.onUpdate = table.TryGetFunctionFromTable("OnUpdate");

	TempScript* returnScript;
	auto it = m_scriptsMap.find(luaFileName.c_str());
	if (it != m_scriptsMap.end())
	{
		it->second.push_back(newScript);
		returnScript = &it->second.back();
	}
	else
	{
		m_scriptsMap.insert({ luaFileName.c_str(), {newScript} });
		it = m_scriptsMap.find(luaFileName.c_str());
		returnScript = &it->second.back();
	}

	return returnScript;
}

//Reloades the script caught by the file watcher
void ScriptManager::ReloadScripts()
{
	#ifndef _DEBUG
		return;
	#endif // _DEBUG

	//Unsafe but want to avoid locking with mutex
	if (s_filesToBeReloaded.size())
	{
		std::lock_guard<std::mutex> lock(s_reloadMutex);

		//Removes any duplicates if there should be any
		s_filesToBeReloaded.erase(std::unique(s_filesToBeReloaded.begin(), s_filesToBeReloaded.end()), s_filesToBeReloaded.end());

		for (int i = 0; i < s_filesToBeReloaded.size(); ++i)
		{
			auto it = m_scriptsMap.find(s_filesToBeReloaded[i].c_str());

			//Should never happen
			if (it == m_scriptsMap.end())
			{
				assert(false);
				return;
			}

			for (int scriptIndex = 0; scriptIndex < it->second.size(); ++scriptIndex)
			{
				TempReloadFile(s_filesToBeReloaded[i], &it->second[scriptIndex]);
			}
			std::cout << s_filesToBeReloaded[i] << "\n";
		}
		s_filesToBeReloaded.clear();
	}
}
