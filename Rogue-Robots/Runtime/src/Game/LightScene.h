#pragma once
#include <DOGEngine.h>

class LightScene : public DOG::Scene
{
public:
	LightScene();
	~LightScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
	void Update();
private:
	DOG::entity AddFrustum(DirectX::SimpleMath::Matrix projetion, DirectX::SimpleMath::Matrix view);
	//DOG::entity AddFrustumDXTK(DirectX::SimpleMath::Matrix projetion, DirectX::SimpleMath::Matrix view);
	DOG::entity AddFace(const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints, DirectX::SimpleMath::Vector3 color);
	DOG::entity AddSphere(DirectX::SimpleMath::Vector3 center, float radius, DirectX::SimpleMath::Vector3 color);
	
	void TiledShadingDebugMenu(bool& open);

};