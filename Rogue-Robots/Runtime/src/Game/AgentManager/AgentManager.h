#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"


class AgentManager
{
	using Vector3 = DirectX::SimpleMath::Vector3;

private:
	// GROUP_BITS must be a factor of 32
	static constexpr u32 GROUP_BITS = 4;
	static constexpr u32 GROUP_RANGE = 1 << GROUP_BITS;
	static constexpr u32 MASK = GROUP_RANGE - 1;

public:
	static constexpr u32 GROUP_SIZE = GROUP_RANGE - 1;	// because agentID = 0 is not unique to group

	[[nodiscard]] static constexpr AgentManager& Get() noexcept
	{
		if (m_notInitialized)
			Initialize();
		return s_amInstance;
	}
	DOG::entity CreateAgent(EntityTypes type, u32 groupID, const Vector3& pos);
	void CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc);
	static Vector3 GenerateRandomVector3(u32 seed, f32 max = 1.0f, f32 min = 0.0f);
	void DestroyLocalAgent(DOG::entity e);
	u32 GroupID(u32 agentID = 0);

private:
	// singelton instance
	static AgentManager s_amInstance;
	static bool m_notInitialized;

	bool m_useNetworking = true;
	std::vector<u32> m_models;
	u32 m_agentIdCounter = 0;
	u32 m_agentKillCounter = 0;

	AgentManager() noexcept;
	~AgentManager() noexcept = default;
	DELETE_COPY_MOVE_CONSTRUCTOR(AgentManager);
	static void Initialize();

	DOG::entity CreateAgentCore(u32 model, u32 groupID, const Vector3& pos, EntityTypes type);	// Setup common for all agents
	u32 GetModel(EntityTypes type);
	u32 GetModel(CreateAndDestroyEntityComponent& entityDesc);
	u32 GenAgentID(u32 groupID);
	void CountAgentKilled(u32 agentID);
};
