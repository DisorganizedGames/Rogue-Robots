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
		//Copy the old entity
		entity entityID = m_luaW->GetIntegerFromTable(scriptData.scriptTable, "EntityID");
		
		//Call OnDestroy for clean up in the lua script
		CallOnDestroy(scriptData);

		//Remove references
		RemoveReferences(scriptData);

		//Reload file
		scriptData.scriptTable = m_luaW->CreateTable();
		m_luaW->AddNumberToTable(scriptData.scriptTable, "EntityID", (int)entityID);
		m_luaW->CreateEnvironment(scriptData.scriptTable, c_pathToScripts + fileName);

		//Get the new functions from the table
		LuaTable table(scriptData.scriptTable, true);
		scriptData.onStartFunction = table.TryGetFunctionFromTable("OnStart");
		scriptData.onUpdateFunction = table.TryGetFunctionFromTable("OnUpdate");
		scriptData.onDestroyFunction = table.TryGetFunctionFromTable("OnDestroy");

		//Call start on the reloaded script (if it exists)!
		if (m_luaW->CheckIfFunctionExist(scriptData.onStartFunction))
			table.CallFunctionOnTable(scriptData.onStartFunction);
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
		m_luaW->RemoveReferenceToFunction(scriptData.onDestroyFunction);
	}

	void ScriptManager::RemoveScriptData(entity entity, bool removeAllEntityScripts, u32 vectorIndex)
	{
		auto entityToScripts = m_entityScripts.find(entity);
		if (entityToScripts != m_entityScripts.end())
		{
			std::vector<StoredScriptData>& storedScriptDataVector = entityToScripts->second;
			for (u32 entityScriptIndex = 0; entityScriptIndex < storedScriptDataVector.size(); ++entityScriptIndex)
			{
				StoredScriptData& storedScriptData = storedScriptDataVector[entityScriptIndex];
				//Remove all scripts if requested otherwise just removes the script type selected
				if (removeAllEntityScripts || vectorIndex == storedScriptData.getScriptData.vectorIndex)
				{
					if (storedScriptData.getScriptData.sorted)
					{
						CallOnDestroy(m_sortedScripts[storedScriptData.getScriptData.vectorIndex][storedScriptData.scriptIndex]);
						RemoveReferences(m_sortedScripts[storedScriptData.getScriptData.vectorIndex][storedScriptData.scriptIndex]);
					}
					else
					{
						CallOnDestroy(m_unsortedScripts[storedScriptData.getScriptData.vectorIndex][storedScriptData.scriptIndex]);
						RemoveReferences(m_unsortedScripts[storedScriptData.getScriptData.vectorIndex][storedScriptData.scriptIndex]);
					}

					//Could be changed for performance if needed
					m_freeScriptPositions.push_back(storedScriptData);
					//Could be changed for performance if needed
					storedScriptDataVector.erase(storedScriptDataVector.begin() + entityScriptIndex);

					--entityScriptIndex;
				}
			}
		}
		else
		{
			std::cout << "Entity does not have any scripts!\n";
			assert(false);
		}
	}

	void ScriptManager::CreateScript(entity entity, const std::string& luaFileName)
	{
		ScriptData scriptData = { entity, -1, -1, -1 };
		scriptData.scriptTable = m_luaW->CreateTable();

		LuaTable table(scriptData.scriptTable, true);
		table.AddIntToTable("EntityID", entity);
		table.CreateEnvironment(c_pathToScripts + luaFileName);
		scriptData.onStartFunction = table.TryGetFunctionFromTable("OnStart");
		scriptData.onUpdateFunction = table.TryGetFunctionFromTable("OnUpdate");
		scriptData.onDestroyFunction = table.TryGetFunctionFromTable("OnDestroy");

		//Check if StartScripts have been called
		if (m_callStartOnCreationOfScripts && m_luaW->CheckIfFunctionExist(scriptData.onStartFunction))
		{
			m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onStartFunction);
		}

		StoredScriptData storedScriptData = {};

		//Find if there already exist a vector for that script type
		auto itScriptToVector = m_scriptToVector.find(luaFileName);
		if (itScriptToVector == m_scriptToVector.end())
		{
			m_unsortedScripts.push_back({ scriptData });
			u32 vectorIndex = (u32)(m_unsortedScripts.size() - 1);

			GetScriptData getScriptData;
			getScriptData.vectorIndex = vectorIndex;
			getScriptData.sorted = false;

			m_scriptToVector.insert({ luaFileName, getScriptData });

			storedScriptData.getScriptData = getScriptData;
			storedScriptData.scriptIndex = 0;
		}
		else
		{
			bool foundFreePlace = false;
			//Get already created position in the vector
			if (m_freeScriptPositions.size())
			{
				//Improvement:
				//Use a map of vectors instead so that we do not have to look through all free positions for all script types
				for (u32 freeScriptIndex = 0; freeScriptIndex < m_freeScriptPositions.size(); ++freeScriptIndex)
				{
					StoredScriptData& freeStoredScriptData = m_freeScriptPositions[freeScriptIndex];

					//Check if there exist an requested free place for the script type
					if (itScriptToVector->second.sorted == freeStoredScriptData.getScriptData.sorted
						&& itScriptToVector->second.vectorIndex == freeStoredScriptData.getScriptData.vectorIndex)
					{
						storedScriptData = freeStoredScriptData;
						//Could be changed for performance if needed
						m_freeScriptPositions.erase(m_freeScriptPositions.begin() + freeScriptIndex);

						//Put the scriptData it in the correct vector
						if (storedScriptData.getScriptData.sorted)
						{
							m_sortedScripts[itScriptToVector->second.vectorIndex][storedScriptData.scriptIndex] = scriptData;
						}
						else
						{
							m_unsortedScripts[itScriptToVector->second.vectorIndex][storedScriptData.scriptIndex] = scriptData;
						}

						foundFreePlace = true;
						break;
					}
				}
			}
			//If there are no free positions for that script type we push it back in the vector
			if (!foundFreePlace)
			{
				if (itScriptToVector->second.sorted)
				{
					m_sortedScripts[itScriptToVector->second.vectorIndex].push_back(scriptData);
					storedScriptData.scriptIndex = (u32)(m_sortedScripts[itScriptToVector->second.vectorIndex].size() - 1);
				}
				else
				{
					m_unsortedScripts[itScriptToVector->second.vectorIndex].push_back(scriptData);
					storedScriptData.scriptIndex = (u32)(m_unsortedScripts[itScriptToVector->second.vectorIndex].size() - 1);
				}
				storedScriptData.getScriptData = itScriptToVector->second;
			}
		}
		
		//Add the script connection to the entity
		auto entityToScripts = m_entityScripts.find(entity);
		if (entityToScripts == m_entityScripts.end())
		{
			m_entityScripts.insert({ entity, { storedScriptData } });
		}
		else
		{
			//Could be changed for performance if needed
			entityToScripts->second.push_back(storedScriptData);
		}
	}

	void ScriptManager::CallOnDestroy(ScriptData& scriptData)
	{
		if (m_luaW->CheckIfFunctionExist(scriptData.onDestroyFunction))
		{
			m_luaW->CallTableLuaFunction(scriptData.scriptTable, scriptData.onDestroyFunction);
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

		m_sortedScriptsHalfwayIndex = 0;
	}

	void ScriptManager::RunLuaFile(const std::string& luaFileName)
	{
		if (std::filesystem::exists(c_pathToScripts + luaFileName))
		{
			m_luaW->RunScript(c_pathToScripts + luaFileName);
		}
		else
		{
			m_luaW->RunScript(luaFileName);
		}
	}

	//Creates a script and runs it
	ScriptComponent& ScriptManager::AddScript(entity entity, const std::string& luaFileName)
	{
		CreateScript(entity, luaFileName);

		//Return script component
		//Return the entity component if it already exists
		bool hasScriptComponent = m_entityManager.HasComponent<ScriptComponent>(entity);
		if (hasScriptComponent)
		{
			return m_entityManager.GetComponent<ScriptComponent>(entity);
		}
		else
			return m_entityManager.AddComponent<ScriptComponent>(entity, entity);
	}

	ScriptData ScriptManager::GetScript(entity entity, const std::string& luaFileName)
	{
		auto itScriptToVector = m_scriptToVector.find(luaFileName);
		if (itScriptToVector == m_scriptToVector.end())
		{
			assert(false);
			return ScriptData();
		}

		std::vector<ScriptData>* scriptDataVector = nullptr;
		if (itScriptToVector->second.sorted)
		{
			scriptDataVector = &m_sortedScripts[itScriptToVector->second.vectorIndex];
		}
		else
		{
			scriptDataVector = &m_unsortedScripts[itScriptToVector->second.vectorIndex];
		}

		//Find the scriptdata and returns it
		for (u32 index = 0; index < scriptDataVector->size(); ++index)
		{
			if (entity == (*scriptDataVector)[index].entity)
				return (*scriptDataVector)[index];
		}

		assert(false);
		return ScriptData();
	}

	void ScriptManager::RemoveScript(entity entity, const std::string& luaFileName)
	{
		auto itScriptToVector = m_scriptToVector.find(luaFileName);
		if (itScriptToVector == m_scriptToVector.end())
		{
			return;
		}

		RemoveScriptData(entity, false, itScriptToVector->second.vectorIndex);
	}

	void ScriptManager::RemoveAllEntityScripts(entity entity)
	{
		if (m_entityManager.HasComponent<ScriptComponent>(entity))
		{
			RemoveScriptData(entity, true);

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
				auto it = m_scriptToVector.find(s_filesToBeReloaded[i]);

				//Sometimes files are reloaded before they even exist as scripts 
				//Return early and clear filesToBeReloaded, should work
				if (it == m_scriptToVector.end())
				{
					std::cout << "File being reloaded but there exist no script of that file\n";
					s_filesToBeReloaded.clear();
					continue;
				}

				bool fileIsReloadedable = TestReloadFile(s_filesToBeReloaded[i]);

				if (fileIsReloadedable)
				{
					GetScriptData getScriptData = it->second;

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
			std::cout << "Reloaded above files\n";

			s_filesToBeReloaded.clear();
		}
	}

	void ScriptManager::StartScripts()
	{
		if (m_callStartOnCreationOfScripts)
			return;

		//Set that StartScripts function have been called
		m_callStartOnCreationOfScripts = true;

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
		MINIPROFILE
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
		m_sortedScriptsHalfwayIndex = 0;
		for (auto& sortData : m_scriptsBeforeSorted)
		{
			if (sortData.sortOrder > 0 && setHalwayIndex)
			{
				m_sortedScriptsHalfwayIndex = (u32)m_sortedScripts.size();
				setHalwayIndex = false;
			}

			auto itScriptToVector = m_scriptToVector.find(sortData.luaFileName);
			if (itScriptToVector == m_scriptToVector.end())
			{
				m_sortedScripts.push_back({});
				u32 vectorIndex = (u32)(m_sortedScripts.size() - 1);
				m_scriptToVector.insert({ sortData.luaFileName, {true, vectorIndex} });
			}
		}

		m_scriptsBeforeSorted.clear();
		m_scriptsBeforeSorted.shrink_to_fit();
	}

	void ScriptManager::RemoveScriptsFromDeferredEntities()
	{
		//Destroy all of the entities with the deferred deletion flag set
		EntityManager::Get().Collect<DeferredDeletionComponent>().Do([&](entity entity, DeferredDeletionComponent&)
			{
				RemoveAllEntityScripts(entity);
			});
	}
}