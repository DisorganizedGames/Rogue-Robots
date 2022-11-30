#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"


class AgentManager
{
	using Vector3 = DirectX::SimpleMath::Vector3;

private:
	static constexpr u32 GROUP_BITS = 7;
	static constexpr u32 GROUP_RANGE = 1 << GROUP_BITS;
	static constexpr u32 MASK = GROUP_RANGE - 1;
	static constexpr u32 NULL_AGENT = u32(-1);

public:
	static constexpr u32 GROUP_SIZE = NULL_AGENT >> GROUP_BITS;	// max agents in a group

	[[nodiscard]] static constexpr AgentManager& Get() noexcept
	{
		if (s_notInitialized)
			Initialize();
		return s_amInstance;
	}
	DOG::entity CreateAgent(EntityTypes type, u32 groupID, const Vector3& pos, SceneComponent::Type scene);
	void CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc);
	static Vector3 GenerateRandomVector3(u32 seed, f32 max = 1.0f, f32 min = 0.0f);
	void DestroyLocalAgent(DOG::entity e, bool local = true);
	static void CreateVillain(const Vector3& position);
	static void CreateScorpioBehaviourTree(DOG::entity agent) noexcept;

	u32 GenAgentID(u32 groupID);
	void CountAgentKilled(u32 agentID);
	u32 GroupID(u32 agentID = NULL_AGENT);

private:
	// singleton instance
	static AgentManager s_amInstance;
	static bool s_notInitialized;

	std::vector<u32> m_models;
	std::array<u32, GROUP_RANGE> m_agentIdCounter{ 0 };
	std::array<u32, GROUP_RANGE> m_agentKillCounter{ 0 };

	AgentManager() noexcept;
	~AgentManager() noexcept = default;
	DELETE_COPY_MOVE_CONSTRUCTOR(AgentManager);
	static void Initialize();

	DOG::entity CreateAgentCore(u32 model, u32 groupID, const Vector3& pos, EntityTypes type);	// Setup common for all agents
	u32 GetModel(EntityTypes type);
	u32 GetModel(CreateAndDestroyEntityComponent& entityDesc);
};
