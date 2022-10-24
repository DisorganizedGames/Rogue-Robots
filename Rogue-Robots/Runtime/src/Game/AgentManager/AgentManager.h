#pragma once
#include <DOGEngine.h>
#include "AgentComponents.h"


class AgentManager
{
	using Vector3 = DirectX::SimpleMath::Vector3;

public:
	entity CreateAgent(/*entityTypeId*/u32 type, Vector3 pos, DOG::SceneType scene);
	void CreateOrDestroyShadowAgent(/*CreateAndDestroyEntityComponent& entityDesc*/);

	AgentManager();
	~AgentManager();

private:
	bool m_useNetworking = true;
	DOG::EntityManager& m_entityManager;
	std::vector<u32> m_models;
	u32 m_agentIdCounter;

	entity CreateAgentCore(u32 model, Vector3 pos, DOG::SceneType scene);	// Setup common for all agents
};
