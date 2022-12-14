#pragma once
#include <DOGEngine.h>
#include "../Scene.h"

class PCGLevelScene : public Scene
{
public:
	PCGLevelScene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, SceneComponent::Type scene, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents, std::string levelName);
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
	const DirectX::SimpleMath::Vector3& GetSpawnblock();
private:
	std::function<std::vector<DOG::entity>(const EntityTypes, SceneComponent::Type, const DirectX::SimpleMath::Vector3&, u8, f32)> m_spawnAgents;
	u8 m_nrOfPlayers;
	std::string m_levelName;
	DirectX::SimpleMath::Vector3 m_spawnblockPos;
};