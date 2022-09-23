#pragma once
#pragma warning(push, 0)
#include "FileWatch/FileWatch.hpp"
#pragma warning(pop)
#include "LuaTable.h"
#include "../ECS/Component.h"
#include "../ECS/EntityManager.h"

namespace DOG
{
	struct TempScript
	{
		std::string scriptName;
		Table luaScript;
		Function onStart;
		Function onUpdate;
	};

	struct ScriptData
	{
		u32 scriptFileID;
		Table scriptTable;
		Function onStartFunction;
		Function onUpdateFunction;
	};

	struct ScriptComponent : public Component<ScriptComponent>
	{
		ScriptComponent(const ScriptData& scriptData) noexcept : scriptData(scriptData) {}
		ScriptData scriptData;
	};

	class ScriptManager
	{
	private:
		u32 m_idCounter;
		std::unordered_map<std::string, u32> m_scriptsIDMap;
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

	public:
		ScriptManager(LuaW* luaW);
		//For lua files which do not require to be scripts
		void RunLuaFile(const std::string& luaFileName);
		//Adds a script and runs it
		ScriptData AddScript(entity entity, const std::string& luaFileName);
		//Reloads script which have changed
		void ReloadScripts();
		//Call start on the scripts which has one
		void StartScripts();
		//Call update on the scripts which has one
		void UpdateScripts();
	};
}