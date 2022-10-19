#pragma once
#pragma warning(push, 0)
#include "FileWatch/FileWatch.hpp"
#pragma warning(pop)
#include "LuaTable.h"
#include "../ECS/Component.h"
#include "../ECS/EntityManager.h"

namespace DOG
{
	struct ScriptData
	{
		entity entity;
		Table scriptTable;
		Function onStartFunction;
		Function onUpdateFunction;
	};

	struct GetScriptData
	{
		bool sorted;
		u32 vectorIndex;
	};

	struct StoredScriptData
	{
		GetScriptData getScriptData;
		u32 scriptIndex;
	};

	struct SortData
	{
		std::string luaFileName;
		int sortOrder;
	};

	struct ScriptComponent
	{
		ScriptComponent(entity entity) noexcept : scriptEntity(entity) {}
		entity scriptEntity;
	};

	class ScriptManager
	{
	private:
		u32 m_idCounter;
		std::unordered_map<std::string, GetScriptData> m_scriptToVector;
		std::vector<std::vector<ScriptData>> m_unsortedScripts;
		std::vector<std::vector<ScriptData>> m_sortedScripts;

		//When removing script, resue the old position for a new script later
		std::vector<StoredScriptData> m_freeScriptPositions;

		//Map from entity to it's scripts
		std::unordered_map<entity, std::vector<StoredScriptData>> m_entityScripts;

		//Only for sorting
		std::vector<SortData> m_scriptsBeforeSorted;

		u32 m_sortedScriptsHalfwayIndex;
		LuaW* m_luaW;
		std::unique_ptr<filewatch::FileWatch<std::filesystem::path>> m_fileWatcher;
		static std::vector<std::string> s_filesToBeReloaded;
		static std::mutex s_reloadMutex;
		const std::string c_pathToScripts = "Assets/LuaScripts/";
		DOG::EntityManager& m_entityManager;

	private:
		static void ScriptFileWatcher(const std::filesystem::path& path, const filewatch::Event changeType);
		void ReloadFile(const std::string& fileName, ScriptData& scriptData);
		bool TestReloadFile(const std::string& fileName);
		void RemoveReferences(ScriptData& scriptData);
		void RemoveScriptData(entity entity, bool removeAllEntityScripts, u32 vectorIndex = 0);
		void CreateScript(entity entity, const std::string& luaFileName);

	public:
		ScriptManager(LuaW* luaW);
		//For lua files which do not require to be scripts
		void RunLuaFile(const std::string& luaFileName);
		//Adds a script and runs it
		ScriptComponent& AddScript(entity entity, const std::string& luaFileName);
		//Adds a script and runs it, also creates the specified component
		template<typename T, class ...Args>
		T& AddScript(entity entity, const std::string& luaFileName, Args&&... args);
		//Get ScriptData
		ScriptData GetScript(entity entity, const std::string& luaFileName);
		//Removes a script from entity
		void RemoveScript(entity entity, const std::string& luaFileName);
		//Removes all scripts from an entity
		void RemoveAllEntityScripts(entity);
		//Reloads script which have changed
		void ReloadScripts();
		//Call start on the scripts which has one
		void StartScripts();
		//Call update on the scripts which has one
		void UpdateScripts();
		//Order Script
		void OrderScript(const std::string& luaFileName, int sortOrder);
		//Sort the ordered scripts
		void SortOrderScripts();
		//
		template<class ...Args>
		void CallFunctionOnAllEntityScripts(entity entity, const std::string& functionName, Args&&... args);

		void RemoveScriptsFromDeferredEntities();
	};

	template<typename T, class ...Args>
	inline T& ScriptManager::AddScript(entity entity, const std::string& luaFileName, Args&&... args)
	{
		CreateScript(entity, luaFileName);

		//Return the created component
		return m_entityManager.AddComponent<T>(entity, std::forward<Args>(args)...);
	}

	template<class ...Args>
	inline void ScriptManager::CallFunctionOnAllEntityScripts(entity entity, const std::string& functionName, Args&&... args)
	{
		if (!m_entityManager.HasComponent<ScriptComponent>(entity))
			return;

		auto entityToScripts = m_entityScripts.find(entity);
		if (entityToScripts != m_entityScripts.end())
		{
			for (u32 index = 0; index < entityToScripts->second.size(); ++index)
			{
				StoredScriptData storedScriptData = entityToScripts->second[index];
				ScriptData scriptData = {};
				if (storedScriptData.getScriptData.sorted)
					scriptData = m_sortedScripts[storedScriptData.getScriptData.vectorIndex][storedScriptData.scriptIndex];
				else
					scriptData = m_unsortedScripts[storedScriptData.getScriptData.vectorIndex][storedScriptData.scriptIndex];

				Function function = m_luaW->TryGetFunctionFromTable(scriptData.scriptTable, functionName);
				if (function.ref != -1)
				{
					LuaTable table(scriptData.scriptTable, true);
					table.CallFunctionOnTable(function, entity, std::forward<Args>(args)...);
				}
			}
		}
		else
		{
			std::cout << "Entity does not exist\n";
			assert(false);
		}
	}
}