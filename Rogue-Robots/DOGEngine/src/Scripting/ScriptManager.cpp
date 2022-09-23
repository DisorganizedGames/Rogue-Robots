#include "ScriptManager.h"

namespace DOG
{
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
	void ScriptManager::ReloadFile(const std::string& fileName, ScriptData& scriptData)
	{
		//Removes the old environment and creates a new one
		m_luaW->RemoveReferenceToTable(scriptData.scriptTable);
		scriptData.scriptTable = m_luaW->CreateTable();
		m_luaW->CreateEnvironment(scriptData.scriptTable, c_pathToScripts + fileName);

		//Remove the old function references
		m_luaW->RemoveReferenceToFunction(scriptData.onStartFunction);
		m_luaW->RemoveReferenceToFunction(scriptData.onUpdateFunction);

		//Get the new functions from the table
		LuaTable table(scriptData.scriptTable, true);
		scriptData.onStartFunction = table.TryGetFunctionFromTable("OnStart");
		scriptData.onUpdateFunction = table.TryGetFunctionFromTable("OnUpdate");
	}

	//Test if we can reload the file and return true/false
	bool ScriptManager::TestReloadFile(const std::string& fileName)
	{
		//Will test lua syntax
		bool failedLoadingFile = m_luaW->TryLoadChunk(c_pathToScripts + fileName);

		if (failedLoadingFile)
			return false;

		//Will only test code outside of functions
		//Maybe add more
		//Maybe should skip this entirely
		//LuaTable testTable(m_luaW);
		//bool failedCreatingEnvironment = testTable.TryCreateEnvironment(c_pathToScripts + fileName);

		//if (failedCreatingEnvironment)
		//	return false;

		return true;
	}

	ScriptManager::ScriptManager(LuaW* luaW) : m_luaW(luaW), m_entityManager(DOG::EntityManager::Get())
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

		m_idCounter = 0;
	}

	void ScriptManager::RunLuaFile(const std::string& luaFileName)
	{
		m_luaW->RunScript(c_pathToScripts + luaFileName);
	}

	//Creates a script and runs it
	ScriptComponent& ScriptManager::AddScript(entity entity, const std::string& luaFileName)
	{
		ScriptData scriptData = {0, -1, -1, -1};
		scriptData.scriptTable = m_luaW->CreateTable();

		LuaTable table(scriptData.scriptTable, true);
		table.CreateEnvironment(c_pathToScripts + luaFileName);
		scriptData.onStartFunction = table.TryGetFunctionFromTable("OnStart");
		scriptData.onUpdateFunction = table.TryGetFunctionFromTable("OnUpdate");

		u32 oldIDCounter = m_idCounter;
		auto it = m_scriptsIDMap.find(luaFileName.c_str());
		if (it == m_scriptsIDMap.end())
		{
			m_scriptsIDMap.insert({ luaFileName.c_str(), {m_idCounter} });
			++m_idCounter;
		}
		scriptData.scriptFileID = oldIDCounter;

		return m_entityManager.AddComponent<ScriptComponent>(entity, scriptData);
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
				auto it = m_scriptsIDMap.find(s_filesToBeReloaded[i].c_str());

				//Should never happen
				if (it == m_scriptsIDMap.end())
				{
					assert(false);
					return;
				}

				bool fileIsReloadedable = TestReloadFile(s_filesToBeReloaded[i]);

				if (fileIsReloadedable)
				{
					u32 scriptID = it->second;
					m_entityManager.Collect<ScriptComponent>().Do([&](ScriptComponent scriptComponent)
						{
							if (scriptComponent.scriptData.scriptFileID == scriptID)
							{
								ReloadFile(s_filesToBeReloaded[i], scriptComponent.scriptData);
							}
						}
					);
				}
				std::cout << s_filesToBeReloaded[i] << "\n";
			}
			s_filesToBeReloaded.clear();
		}
	}

	void ScriptManager::StartScripts()
	{
		m_entityManager.Collect<ScriptComponent>().Do([&](ScriptComponent scriptComponent)
			{
				if (scriptComponent.scriptData.onStartFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptComponent.scriptData.scriptTable, scriptComponent.scriptData.onStartFunction);
			}
		);
	}

	void ScriptManager::UpdateScripts()
	{
		m_entityManager.Collect<ScriptComponent>().Do([&](ScriptComponent scriptComponent)
			{
				if (scriptComponent.scriptData.onUpdateFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptComponent.scriptData.scriptTable, scriptComponent.scriptData.onUpdateFunction);
			}
		);
	}
}