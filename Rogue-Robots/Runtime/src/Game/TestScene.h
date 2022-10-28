#pragma once
#include <DOGEngine.h>

class TestScene : public DOG::Scene
{
public:
	TestScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

	void CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position);
};