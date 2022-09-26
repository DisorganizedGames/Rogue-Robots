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

	void ScriptManager::RemoveReferences(ScriptData& scriptData)
	{
		m_luaW->RemoveReferenceToTable(scriptData.scriptTable);
		m_luaW->RemoveReferenceToFunction(scriptData.onStartFunction);
		m_luaW->RemoveReferenceToFunction(scriptData.onUpdateFunction);
	}

	void ScriptManager::RemoveScriptData(std::vector<ScriptData>& scriptVector, entity entity)
	{
		for (u32 index = 0; index < scriptVector.size(); ++index)
		{
			if (scriptVector[index].entity == entity)
			{
				RemoveReferences(scriptVector[index]);
				scriptVector.erase(scriptVector.begin() + index);
				--index;
			}
		}
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
		ScriptData scriptData = { entity, -1, -1, -1};
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
		else
		{
			oldIDCounter = it->second;
		}

		auto itScriptToVector = m_scriptToVector.find(oldIDCounter);
		if (itScriptToVector == m_scriptToVector.end())
		{
			m_unsortedScripts.push_back({scriptData});
			u32 vectorIndex = (u32)(m_unsortedScripts.size() - 1);
			m_scriptToVector.insert({ oldIDCounter, {false, vectorIndex}});
		}
		else
		{
			if (itScriptToVector->second.sorted)
			{
				m_sortedScripts[itScriptToVector->second.vectorIndex].push_back(scriptData);
			}
			else
			{
				m_unsortedScripts[itScriptToVector->second.vectorIndex].push_back(scriptData);
			}
		}

		bool hasScriptComponent = m_entityManager.HasComponent<ScriptComponent>(entity);
		ScriptComponent scriptComponent(0);
		if (hasScriptComponent)
		{
			scriptComponent = m_entityManager.GetComponent<ScriptComponent>(entity);
		}
		else
			scriptComponent = m_entityManager.AddComponent<ScriptComponent>(entity, entity);
		return scriptComponent;
	}

	void ScriptManager::RemoveScript(entity entity, const std::string& luaFileName)
	{
		auto it = m_scriptsIDMap.find(luaFileName.c_str());
		if (it == m_scriptsIDMap.end())
		{
			return;
		}

		auto itScriptToVector = m_scriptToVector.find(it->second);
		if (itScriptToVector == m_scriptToVector.end())
		{
			return;
		}

		if (itScriptToVector->second.sorted)
		{
			auto vector = m_sortedScripts[itScriptToVector->second.vectorIndex];
			RemoveScriptData(vector, entity);
		}
		else
		{
			auto vector = m_unsortedScripts[itScriptToVector->second.vectorIndex];
			RemoveScriptData(vector, entity);
		}
	}

	void ScriptManager::RemoveAllEntityScripts(entity entity)
	{
		if (m_entityManager.HasComponent<ScriptComponent>(entity))
		{
			for (auto& vector : m_sortedScripts)
			{
				RemoveScriptData(vector, entity);
			}
			for (auto& vector : m_unsortedScripts)
			{
				RemoveScriptData(vector, entity);
			}

			m_entityManager.RemoveComponent<ScriptComponent>(entity);
		}
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

					GetScriptData getScriptData = m_scriptToVector[scriptID];

					if (getScriptData.sorted)
					{
						for (auto& scriptData : m_sortedScripts[getScriptData.vectorIndex])
						{
							ReloadFile(s_filesToBeReloaded[i], scriptData);
						}
					}
					else
					{
						for (auto& scriptData : m_unsortedScripts[getScriptData.vectorIndex])
						{
							ReloadFile(s_filesToBeReloaded[i], scriptData);
						}
					}
				}
				std::cout << s_filesToBeReloaded[i] << "\n";
			}
			s_filesToBeReloaded.clear();
		}
	}

	void ScriptManager::StartScripts()
	{
		//Run the scripts which should happen first!
		for (u32 index = 0; index < m_sortedScriptsHalfwayIndex; ++index)
		{
			for (auto& scriptData : m_sortedScripts[index])
			{
				if (scriptData.onStartFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onStartFunction);
			}
		}
		//Run the scripts which does not have an order!
		for (u32 index = 0; index < m_unsortedScripts.size(); ++index)
		{
			for (auto& scriptData : m_unsortedScripts[index])
			{
				if (scriptData.onStartFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onStartFunction);
			}
		}
		//Run the scripts which should happen last!
		for (u32 index = m_sortedScriptsHalfwayIndex; index < m_sortedScripts.size(); ++index)
		{
			for (auto& scriptData : m_sortedScripts[index])
			{
				if (scriptData.onStartFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onStartFunction);
			}
		}
	}

	void ScriptManager::UpdateScripts()
	{
		//Run the scripts which should happen first!
		for (u32 index = 0; index < m_sortedScriptsHalfwayIndex; ++index)
		{
			for (auto& scriptData : m_sortedScripts[index])
			{
				if (scriptData.onUpdateFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onUpdateFunction);
			}
		}
		//Run the scripts which does not have an order!
		for (u32 index = 0; index < m_unsortedScripts.size(); ++index)
		{
			for (auto& scriptData : m_unsortedScripts[index])
			{
				if (scriptData.onUpdateFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onUpdateFunction);
			}
		}
		//Run the scripts which should happen last!
		for (u32 index = m_sortedScriptsHalfwayIndex; index < m_sortedScripts.size(); ++index)
		{
			for (auto& scriptData : m_sortedScripts[index])
			{
				if (scriptData.onUpdateFunction.ref != -1)
					m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onUpdateFunction);
			}
		}
	}

	void ScriptManager::OrderScript(const std::string& luaFileName, int sortOrder)
	{
		m_scriptsBeforeSorted.push_back({luaFileName, sortOrder });
	}

	void ScriptManager::SortOrderScripts()
	{
		if (m_sortedScripts.size() > 0)
			return;

		std::sort(m_scriptsBeforeSorted.begin(), m_scriptsBeforeSorted.end(), [](SortData data1, SortData data2) {
			return (data1.sortOrder < data2.sortOrder);
			});

		bool setHalwayIndex = true;
		for (auto& sortData : m_scriptsBeforeSorted)
		{
			if (sortData.sortOrder > 0 && setHalwayIndex)
			{
				m_sortedScriptsHalfwayIndex = (u32)m_sortedScripts.size();
				setHalwayIndex = false;
			}

			u32 oldIDCounter = m_idCounter;
			auto it = m_scriptsIDMap.find(sortData.luaFileName.c_str());
			if (it == m_scriptsIDMap.end())
			{
				m_scriptsIDMap.insert({ sortData.luaFileName.c_str(), {m_idCounter} });
				++m_idCounter;
			}

			auto itScriptToVector = m_scriptToVector.find(oldIDCounter);
			if (itScriptToVector == m_scriptToVector.end())
			{
				m_sortedScripts.push_back({});
				u32 vectorIndex = (u32)(m_sortedScripts.size() - 1);
				m_scriptToVector.insert({ oldIDCounter, {true, vectorIndex} });
			}
		}

		m_scriptsBeforeSorted.clear();
		m_scriptsBeforeSorted.shrink_to_fit();
	}
}