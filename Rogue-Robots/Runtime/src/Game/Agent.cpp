#include "Agent.h"
using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

Agent::Agent() : m_entityManager(EntityManager::Get()), m_agentEntity(m_entityManager.CreateEntity())
{
	// Set default components
	m_entityManager.AddComponent<TransformComponent>(m_agentEntity, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1));
	// Attach correct script
	LuaMain::GetScriptManager()->AddScript(m_agentEntity, "Agent.lua");
	m_entityManager.AddComponent<NetworkTransform>(m_agentEntity).objectId  = m_agentEntity;
}

Agent::~Agent()
{
	LuaMain::GetScriptManager()->RemoveAllEntityScripts(m_agentEntity);
	m_entityManager.EntityDeferredDestruction(m_agentEntity);
}
