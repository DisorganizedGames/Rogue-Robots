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
	static void DestroyLocalAgent(DOG::entity e);

	AgentManager();
	~AgentManager() = default;

private:
	// GROUP_BITS must be a factor of 32
	static constexpr u32 GROUP_BITS = 4;
	static constexpr u32 GROUP_SIZE = 1 << GROUP_BITS;	//16
	static constexpr u32 MASK = GROUP_SIZE - 1;

	bool m_useNetworking = true;
	DOG::EntityManager& m_entityManager;
	std::vector<u32> m_models;
	u32 m_agentIdCounter;
	u32 m_agentKillCounter;

	DOG::entity CreateAgentCore(u32 model, const Vector3& pos, EntityTypes type);	// Setup common for all agents
	u32 GetModel(EntityTypes type);
	u32 GetModel(CreateAndDestroyEntityComponent& entityDesc);
	u32 GenAgentID(u32 groupID);
	void CountAgentKilled(u32 agentID);
	u32 GroupID(u32 agentID = 0);
};
