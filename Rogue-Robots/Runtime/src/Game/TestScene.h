#pragma once
#include <DOGEngine.h>
#include "Scene.h"

class TestScene : public Scene
{
public:
	TestScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

	void CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position);
	void CreateMissilePickup(DirectX::SimpleMath::Vector3 position);
	void CreateGrenadePickup(DirectX::SimpleMath::Vector3 position);
};