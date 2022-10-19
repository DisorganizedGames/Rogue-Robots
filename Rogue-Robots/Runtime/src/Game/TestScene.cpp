#include "TestScene.h"
#include "GameComponent.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

TestScene::TestScene() : Scene(SceneType::TestScene)
{

}

void TestScene::SetUpScene()
{
	auto& am = DOG::AssetManager::Get();


	u32 sphereID = am.LoadShapeAsset(Shape::sphere, 8, 8);
	entity sphereEntity = CreateEntity();
	AddComponent<TransformComponent>(sphereEntity, Vector3(30, 20, 30));
	AddComponent<ModelComponent>(sphereEntity, sphereID);



	u32 greenCubeID = am.LoadModelAsset("Assets/Models/Temporary_Assets/green_cube.glb", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	entity entity2 = CreateEntity();
	AddComponent<ModelComponent>(entity2, greenCubeID);
	AddComponent<TransformComponent>(entity2, Vector3(-4, -2, 5), Vector3(0.1f, 0, 0));
	AddComponent<MeshColliderComponent>(entity2, entity2, greenCubeID);

	u32 blueCubeID = am.LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");
	entity entity3 = CreateEntity();
	AddComponent<ModelComponent>(entity3, blueCubeID);
	auto& t3 = AddComponent<TransformComponent>(entity3);

	t3.SetPosition({ 4, 2, 5 });
	t3.SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<SphereColliderComponent>(entity3, entity3, 1.0f, true);
	AddComponent<RigidbodyComponent>(entity3, entity3);


	entity entity80 = CreateEntity();
	AddComponent<ModelComponent>(entity80, blueCubeID);
	auto& t80 = AddComponent<TransformComponent>(entity80);
	t80.worldMatrix = t3.worldMatrix;
	AddComponent<SphereColliderComponent>(entity80, entity80, 1.0f, true);
	AddComponent<RigidbodyComponent>(entity80, entity80);

	u32 mixamoID = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/walkmix.fbx");
	entity entity5 = CreateEntity();
	AddComponent<ModelComponent>(entity5, mixamoID);
	AddComponent<TransformComponent>(entity5, Vector3(0, -2, 5), Vector3(0, 0, 0), Vector3(0.02f, 0.02f, 0.02f));
	AddComponent<AnimationComponent>(entity5).offset = 0;
	AddComponent<CapsuleColliderComponent>(entity5, entity5, 1.0f, 1.0f, false);


	// Create some shapes
	{
		std::vector<u32> shapes;
		u32 tessFactor[3] = { 1, 10, 100 };
		for (u32 i = 0; i < 3; i++) // 3 sheets
			shapes.push_back(am.LoadShapeAsset(Shape::sheet, tessFactor[i]));
		for (u32 i = 0; i < 3; i++) // 3 spheres
			shapes.push_back(am.LoadShapeAsset(Shape::sphere, 2 + tessFactor[i], 2 + tessFactor[i]));
		for (u32 i = 0; i < 3; i++) // 3 cones
			shapes.push_back(am.LoadShapeAsset(Shape::cone, 2 + tessFactor[i], 2 + tessFactor[i]));
		for (u32 i = 0; i < 3; i++) // 3 prisms
			shapes.push_back(am.LoadShapeAsset(Shape::prism, 2 + tessFactor[i], 2 + tessFactor[i]));

		for (i32 i = 0; i < 4; i++)
		{
			for (i32 j = 0; j < 3; j++)
			{
				entity e = CreateEntity();
				AddComponent<ModelComponent>(e, shapes[i * 3 + j]);
				AddComponent<TransformComponent>(e, Vector3(f32(-3 + j * 3), (f32)(-4.5f + i * 3), 10), Vector3(0, 0, 0));
			}
		}
		entity xAxis = CreateEntity();
		AddComponent<ModelComponent>(xAxis, shapes[9]);
		AddComponent<TransformComponent>(xAxis, Vector3(0, 0, 0), Vector3(0, 0, XM_PIDIV2), Vector3(0.02f, 100, 0.02f));
		entity yAxis = CreateEntity();
		AddComponent<ModelComponent>(yAxis, shapes[10]);
		AddComponent<TransformComponent>(yAxis, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0.02f, 100, 0.02f));
		entity zAxis = CreateEntity();
		AddComponent<ModelComponent>(zAxis, shapes[11]);
		AddComponent<TransformComponent>(zAxis, Vector3(0, 0, 0), Vector3(XM_PIDIV2, 0, 0), Vector3(0.02f, 100, 0.02f));
	}


	entity isoSphereEntity = CreateEntity();
	AddComponent<ModelComponent>(isoSphereEntity, am.LoadModelAsset("Assets/Models/Temporary_Assets/iso_sphere.glb"));
	AddComponent<TransformComponent>(isoSphereEntity, Vector3(20, 10, 30)).SetScale({ 2,2,2 });
	auto& isoSphereLight = AddComponent<PointLightComponent>(isoSphereEntity);
	isoSphereLight.color = Vector3(0.1f, 1.0f, 0.2f);
	isoSphereLight.strength = 30;
	isoSphereLight.handle = LightManager::Get().AddPointLight(
		PointLightDesc
		{
			.position = s_entityManager.GetComponent<TransformComponent>(isoSphereEntity).GetPosition(),
			.color = isoSphereLight.color,
			.strength = isoSphereLight.strength
		},
		LightUpdateFrequency::PerFrame);

	auto& lerpAnimator = AddComponent<LerpAnimateComponent>(isoSphereEntity);
	lerpAnimator.origin = s_entityManager.GetComponent<TransformComponent>(isoSphereEntity).GetPosition();
	lerpAnimator.target = lerpAnimator.origin + Vector3(0, 5, 0);
	lerpAnimator.loops = -1;
	lerpAnimator.scale = 0.2;

	u32 magentaCubeID = am.LoadModelAsset("Assets/Models/Temporary_Assets/magenta_cube.glb");
	entity doorTest = CreateEntity();
	AddComponent<DoorComponent>(doorTest).roomId = 0;
	AddComponent<TransformComponent>(doorTest, Vector3(25, 6, 15));
	AddComponent<ModelComponent>(doorTest, magentaCubeID);


	// Setup lights

	// Default lights
	u32 xOffset = 18;
	u32 zOffset = 18;
	for (u32 i = 0; i < 3; ++i)
	{
		for (u32 x = 0; x < 3; ++x)
		{
			auto pdesc = PointLightDesc();
			pdesc.position = { xOffset + (f32)i * 7.f, 8.f, zOffset + (f32)x * 7.f };
			pdesc.color = { 1.f, 0.f, 0.f };
			pdesc.strength = 10.f;
			LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::Never);

			auto dd = SpotLightDesc();
			dd.position = { xOffset + (f32)i * 7.f, 16.f, zOffset + (f32)x * 7.f };
			dd.color = { 0.f, 0.f, 1.f };
			dd.direction = { 0.f, 1.f, 0.f };
			dd.strength = 1.f;
			LightManager::Get().AddSpotLight(dd, LightUpdateFrequency::Never);
		}
	}

	// Moving light
	LightHandle pointLight = LightManager::Get().AddPointLight(PointLightDesc(), LightUpdateFrequency::PerFrame);
	entity movingPointLight = CreateEntity();
	AddComponent<TransformComponent>(movingPointLight, Vector3(12, 10, 10), Vector3(0, 0, 0), Vector3(1.f));
	AddComponent<PointLightComponent>(movingPointLight, pointLight, Vector3(1.f, 1.f, 0.f), 5.f);
	auto& lightAnimation = AddComponent<LerpAnimateComponent>(movingPointLight);
	lightAnimation.origin = s_entityManager.GetComponent<TransformComponent>(movingPointLight).GetPosition();
	lightAnimation.target = lightAnimation.origin + Vector3(0, 0, 30);
	lightAnimation.loops = -1;
	lightAnimation.scale = 0.3;
}
