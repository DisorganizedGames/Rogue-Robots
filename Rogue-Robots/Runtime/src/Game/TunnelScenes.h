#pragma once
#include <DOGEngine.h>
#include "Scene.h"


class TunnelRoom0Scene : public Scene
{
public:
	TunnelRoom0Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents);
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> m_spawnAgents;
	u8 m_nrOfPlayers;
};


class TunnelRoom1Scene : public Scene
{
public:
	TunnelRoom1Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents);
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> m_spawnAgents;
	u8 m_nrOfPlayers;
};


class TunnelRoom2Scene : public Scene
{
public:
	TunnelRoom2Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents);
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> m_spawnAgents;
	u8 m_nrOfPlayers;
};


class TunnelRoom3Scene : public Scene
{
public:
	TunnelRoom3Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents);
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> m_spawnAgents;
	u8 m_nrOfPlayers;
};