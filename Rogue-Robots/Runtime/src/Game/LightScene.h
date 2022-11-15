#pragma once
#include <DOGEngine.h>
#include "Scene.h"
#include "FakeComputeLightCulling.h"

class LightScene : public Scene
{
public:
	LightScene();
	~LightScene();
	void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) override;
	void Update();
	static std::vector<DirectX::SimpleMath::Vector4> ExtractPlanes(DirectX::SimpleMath::Matrix projection, DirectX::SimpleMath::Matrix view, int resX, int resY, int tileSize, Vector2i tile);
	DOG::entity AddFrustum(DirectX::SimpleMath::Matrix projection, DirectX::SimpleMath::Matrix view);
	DOG::entity AddFrustum(DirectX::SimpleMath::Vector4 leftPlane, DirectX::SimpleMath::Vector4 rightPlane,
		DirectX::SimpleMath::Vector4 bottomPlane, DirectX::SimpleMath::Vector4 topPlane,
		DirectX::SimpleMath::Vector4 nearPlane, DirectX::SimpleMath::Vector4 farPlane);
	DOG::entity AddFace(const std::vector<DirectX::SimpleMath::Vector3>& vertexPoints, const std::pair<DOG::MaterialHandle, DOG::MaterialDesc>& mat);
	DOG::entity AddSphere(DirectX::SimpleMath::Vector3 center, float radius, DirectX::SimpleMath::Vector3 color);
	
private:
	std::pair<DOG::MaterialHandle, DOG::MaterialDesc> m_rgbMats[3];


	void TiledShadingDebugMenu(bool& open);

	void LightCullingDebugMenu(bool& open);
	bool m_cullingResultWindowOpen = false;
	void CullingResultWindow();
	FakeCompute m_compute;
	bool m_testWindowOpen = false;

};