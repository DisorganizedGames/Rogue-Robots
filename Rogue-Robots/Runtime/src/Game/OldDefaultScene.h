#pragma once
#include <DOGEngine.h>
#include "Scene.h"

class OldDefaultScene : public Scene
{
public:
	OldDefaultScene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, SceneComponent::Type scene, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents);
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

private:
	std::function<std::vector<DOG::entity>(const EntityTypes, SceneComponent::Type scene, const DirectX::SimpleMath::Vector3&, u8, f32)> m_spawnAgents;
	u8 m_nrOfPlayers;
};