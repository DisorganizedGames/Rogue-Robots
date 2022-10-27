#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"


class AgentManager
{
	using Vector3 = DirectX::SimpleMath::Vector3;

public:
	DOG::entity CreateAgent(EntityTypes type, const Vector3& pos);
	void CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc);
	static Vector3 GenerateRandomVector3(u32 seed, f32 max = 1.0f, f32 min = 0.0f);

	AgentManager();
	~AgentManager() = default;

private:
	bool m_useNetworking = true;
	DOG::EntityManager& m_entityManager;
	std::vector<u32> m_models;
	u32 m_agentIdCounter;

	DOG::entity CreateAgentCore(u32 model, const Vector3& pos, EntityTypes type);	// Setup common for all agents
};
