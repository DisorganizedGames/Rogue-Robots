#include "Agent.h"
using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

Agent::Agent() : m_entityManager(EntityManager::Get())
{

}

Agent::~Agent()
{
	//LuaMain::GetScriptManager()->RemoveAllEntityScripts(m_agentEntity);
	//m_entityManager.DeferredEntityDestruction(m_agentEntity);
}

entity Agent::MakeAgent(DOG::entity e) noexcept
{
	// Set default components
	if (!m_entityManager.HasComponent<TransformComponent>(e))
		m_entityManager.AddComponent<TransformComponent>(e);

	if (m_useNetworking)
	{
		if (!m_entityManager.HasComponent<NetworkTransform>(e))
			m_entityManager.AddComponent<NetworkTransform>(e).objectId = e;
	}

	// Attach correct script
	LuaMain::GetScriptManager()->AddScript(e, "Agent.lua");
	return e;
}
