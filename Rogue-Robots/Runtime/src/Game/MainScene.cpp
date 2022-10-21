#include "MainScene.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;


MainScene::MainScene() : Scene(SceneType::MainScene)
{

}

void MainScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
	{
		auto entities = func();
		for (entity e : entities)
			AddComponent<SceneComponent>(e, m_sceneType);
	}
}
