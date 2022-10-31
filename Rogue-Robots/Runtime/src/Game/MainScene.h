#pragma once
#include <DOGEngine.h>

class MainScene : public DOG::Scene
{
public:
	MainScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;

	void CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position);
	void CreateMissilePickup(DirectX::SimpleMath::Vector3 position);
	void CreateGrenadePickup(DirectX::SimpleMath::Vector3 position);
	void CreateMaxHealthBoostPickup(DirectX::SimpleMath::Vector3 position);
	void CreateFrostModificationPickup(DirectX::SimpleMath::Vector3 position);
	void CreateLight(DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Vector3 color = {1,1,1}, float strength = 1.0f);
};