#include "TestScene.h"

using namespace DOG;

TestScene::TestScene() : Scene(SceneType::TestScene)
{

}
void TestScene::SetUpScene()
{
	u32 sphereID = AssetManager::Get().LoadShapeAsset(Shape::sphere, 8, 8);
	entity e = CreateEntity();
	s_entityManager.AddComponent<TransformComponent>(e, DirectX::SimpleMath::Vector3(30, 20, 30));
	s_entityManager.AddComponent<ModelComponent>(e, sphereID);
}
