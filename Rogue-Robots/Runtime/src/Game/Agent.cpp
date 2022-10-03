#include "Agent.h"
using namespace DOG;
using namespace DirectX;

Agent::Agent() : m_entityManager(EntityManager::Get()), m_agentEntity(m_entityManager.CreateEntity())
{
	// Attach correct script
	LuaMain::GetScriptManager()->AddScript(m_agentEntity, "Agent.lua");
}

Agent::~Agent()
{
	LuaMain::GetScriptManager()->RemoveAllEntityScripts(m_agentEntity);
	m_entityManager.DestroyEntity(m_agentEntity);
}
