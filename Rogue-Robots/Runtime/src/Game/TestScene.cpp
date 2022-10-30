#include "TestScene.h"
#include "GameComponent.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

TestScene::TestScene() : Scene(SceneComponent::Type::TestScene)
{

}

void TestScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for(auto& func : entityCreators)
	{
		auto entities = func();
		for (entity e : entities)
			AddComponent<SceneComponent>(e, m_sceneType);
	}

	auto& am = DOG::AssetManager::Get();

	u32 greenCubeID = am.LoadModelAsset("Assets/Models/Temporary_Assets/green_cube.glb", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	u32 magentaCubeID = am.LoadModelAsset("Assets/Models/Temporary_Assets/magenta_cube.glb");
	u32 blueCubeID = am.LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");
	u32 mixamoID = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/walkmix.fbx");
	u32 isoSphereID = am.LoadModelAsset("Assets/Models/Temporary_Assets/iso_sphere.glb");
	u32 medkitID = am.LoadModelAsset("Assets/Models/Temporary_Assets/medkit.glb");

	u32 sphereID = am.LoadShapeAsset(Shape::sphere, 8, 8);
	entity sphereEntity = CreateEntity();
	AddComponent<TransformComponent>(sphereEntity, Vector3(30, 20, 30));
	AddComponent<ModelComponent>(sphereEntity, sphereID);
	AddComponent<ShadowReceiverComponent>(sphereEntity);

	entity entity2 = CreateEntity();
	AddComponent<ModelComponent>(entity2, greenCubeID);
	AddComponent<TransformComponent>(entity2, Vector3(-4, -2, 5), Vector3(0.1f, 0, 0));
	AddComponent<MeshColliderComponent>(entity2, entity2, greenCubeID);
	AddComponent<ShadowReceiverComponent>(entity2);

	entity entity3 = CreateEntity();
	AddComponent<ModelComponent>(entity3, blueCubeID);
	auto& t3 = AddComponent<TransformComponent>(entity3);
	t3.SetPosition({ 4, 2, 5 });
	t3.SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<SphereColliderComponent>(entity3, entity3, 1.0f, true);
	AddComponent<RigidbodyComponent>(entity3, entity3);
	AddComponent<ShadowReceiverComponent>(entity3);

	entity entity80 = CreateEntity();
	AddComponent<ModelComponent>(entity80, blueCubeID);
	auto& t80 = AddComponent<TransformComponent>(entity80);
	t80.worldMatrix = t3.worldMatrix;
	AddComponent<SphereColliderComponent>(entity80, entity80, 1.0f, true);
	AddComponent<RigidbodyComponent>(entity80, entity80);
	AddComponent<ShadowReceiverComponent>(entity80);

	entity entity81 = CreateEntity();
	AddComponent<ModelComponent>(entity81, magentaCubeID);
	AddComponent<TransformComponent>(entity81, Vector3(39, 30, 40));
	AddComponent<BoxColliderComponent>(entity81, entity81, Vector3(1, 1, 1), true);
	AddComponent<RigidbodyComponent>(entity81, entity81);
	AddComponent<ShadowReceiverComponent>(entity81);

	entity entity5 = CreateEntity();
	AddComponent<ModelComponent>(entity5, mixamoID);
	AddComponent<TransformComponent>(entity5, Vector3(0, -2, 5), Vector3(0, 0, 0), Vector3(0.02f, 0.02f, 0.02f));
	AddComponent<AnimationComponent>(entity5).offset = 0;
	AddComponent<CapsuleColliderComponent>(entity5, entity5, 1.0f, 1.0f, false);
	AddComponent<ShadowReceiverComponent>(entity5);


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
				AddComponent<ShadowReceiverComponent>(e);
			}
		}
		entity xAxis = CreateEntity();
		AddComponent<ModelComponent>(xAxis, shapes[9]);
		AddComponent<TransformComponent>(xAxis, Vector3(0, 0, 0), Vector3(0, 0, XM_PIDIV2), Vector3(0.02f, 100, 0.02f));
		AddComponent<ShadowReceiverComponent>(xAxis);
		entity yAxis = CreateEntity();
		AddComponent<ModelComponent>(yAxis, shapes[10]);
		AddComponent<TransformComponent>(yAxis, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0.02f, 100, 0.02f));
		AddComponent<ShadowReceiverComponent>(yAxis);
		entity zAxis = CreateEntity();
		AddComponent<ModelComponent>(zAxis, shapes[11]);
		AddComponent<TransformComponent>(zAxis, Vector3(0, 0, 0), Vector3(XM_PIDIV2, 0, 0), Vector3(0.02f, 100, 0.02f));
		AddComponent<ShadowReceiverComponent>(zAxis);
	}

	entity isoSphereEntity = CreateEntity();
	AddComponent<ModelComponent>(isoSphereEntity, isoSphereID);
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

	entity doorTest = CreateEntity();
	AddComponent<DoorComponent>(doorTest).roomId = 0;
	AddComponent<TransformComponent>(doorTest, Vector3(25, 6, 15));
	u32 doorModelID = am.LoadModelAsset("Assets/Models/Temporary_Assets/Door.gltf", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	AddComponent<ModelComponent>(doorTest, doorModelID);
	AddComponent<ShadowReceiverComponent>(doorTest);


	entity passiveItemTest = CreateEntity();
	AddComponent<PassiveItemComponent>(passiveItemTest).type = PassiveItemComponent::Type::Template;
	AddComponent<PickupComponent>(passiveItemTest);
	AddComponent<ModelComponent>(passiveItemTest, medkitID);
	AddComponent<TransformComponent>(passiveItemTest, Vector3(25, 15, 30));
	AddComponent<BoxColliderComponent>(passiveItemTest, passiveItemTest, Vector3(0.2f, 0.2f, 0.2f), true);
	AddComponent<RigidbodyComponent>(passiveItemTest, passiveItemTest, false);
	AddComponent<ShadowReceiverComponent>(passiveItemTest);
	LuaMain::GetScriptManager()->AddScript(passiveItemTest, "Pickupable.lua");

	CreateTrampolinePickup(Vector3(23.0f, 6.0f, 30.0f));
	CreateTrampolinePickup(Vector3(20.0f, 6.0f, 35.0f));
	CreateTrampolinePickup(Vector3(55.0f, 6.0f, 35.0f));
	CreateTrampolinePickup(Vector3(60.0f, 6.0f, 35.0f));
	CreateTrampolinePickup(Vector3(47.0f, 6.0f, 35.0f));
	CreateTrampolinePickup(Vector3(18.0f, 6.0f, 35.0f));
	CreateMissilePickup(Vector3(15.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(18.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(24.0f, 6.0f, 17.0f));

	CreateMissilePickup(Vector3(26.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(28.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(30.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(32.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(34.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(36.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(38.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(40.0f, 6.0f, 17.0f));

	CreateGrenadePickup(Vector3(42.0f, 6.0f, 17.0f));

	// Setup lights
	// Default lights

	// TODO add a way of deleting static lights
	
	u32 xOffset = 15;
	u32 zOffset = 18;
	for (u32 i = 0; i < 3; ++i)
	{
		for (u32 x = 0; x < 3; ++x)
		{
			auto e = CreateEntity();

			auto pdesc = PointLightDesc();
			pdesc.position = { xOffset + (f32)i * 15.f, 12.f, zOffset + (f32)x * 15.f };
			pdesc.color = { 0.f, 0.3f, 0.7f };
			pdesc.strength = 35.f;
			auto& plc = AddComponent<PointLightComponent>(e);
			plc.handle = LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::Never);
			plc.color = pdesc.color;
			plc.strength = pdesc.strength;



			//auto e2 = CreateEntity();
			//auto dd = SpotLightDesc();
			//dd.position = { xOffset + (f32)i * 7.f, 16.f, zOffset + (f32)x * 7.f };
			//dd.color = { 0.f, 0.f, 1.f };
			//dd.direction = { 0.f, 1.f, 0.f };
			//dd.strength = 1.f;
			//auto& slc = AddComponent<SpotLightComponent>(e2);
			//slc.handle = LightManager::Get().AddSpotLight(dd, LightUpdateFrequency::Never);
			//slc.color = dd.color;
			//slc.strength = dd.strength;
			//slc.direction = dd.direction;
			//slc.cutoffAngle = dd.cutoffAngle;
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


	// Load custom mesh and custom material (one sub-mesh + one material)
	// Material is modifiable
	{
		// Load mesh
		MeshDesc md{};
		SubmeshMetadata smMd{};

		auto mod = ShapeCreator(Shape::cone, 4).GetResult();
		md.indices = mod->mesh.indices;
		md.submeshData = mod->submeshes;

		for (const auto& [k, _] : mod->mesh.vertexData)
			md.vertexDataPerAttribute[k] = mod->mesh.vertexData[k];
		auto meshData = CustomMeshManager::Get().AddMesh(md);

		// Load material
		MaterialDesc d{};
		d.albedoFactor = { 1.f, 0.5f, 0.f };
		d.roughnessFactor = 0.3f;
		d.metallicFactor = 0.6f;
		auto mat = CustomMaterialManager::Get().AddMaterial(d);

		auto testE = CreateEntity();
		AddComponent<TransformComponent>(testE,
			DirectX::SimpleMath::Vector3{ 25.f, 10.f, 25.f },
			DirectX::SimpleMath::Vector3{},
			DirectX::SimpleMath::Vector3{ 3.f, 3.f, 3.f });
		AddComponent<SubmeshRenderer>(testE, meshData.first, mat, d);
		auto& lerpColor = AddComponent<LerpColorComponent>(testE);
		lerpColor.origin = Vector3(d.albedoFactor);
		lerpColor.target = Vector3(1, 1, 1) - lerpColor.origin;
		lerpColor.loops = -1;
		lerpColor.scale = 0.7;

		AddComponent<ShadowReceiverComponent>(testE);
	}
	
	/* Set up music player */
	entity musicPlayer = CreateEntity();
	u32 rogueRobotsMusic = AssetManager::Get().LoadAudio("Assets/Audio/RogueRobots.wav");
	AddComponent<AudioComponent>(musicPlayer) = {
		.assetID = rogueRobotsMusic,
		.loopStart = 15.f,
		.loopEnd = 110.f,
		.shouldPlay = false,
		.loop = true,
	};
}

void TestScene::CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position)
{
	u32 trampolineID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Trampoline.glb");

	entity trampolineEntity = CreateEntity();
	AddComponent<ActiveItemComponent>(trampolineEntity).type = ActiveItemComponent::Type::Trampoline;
	AddComponent<PickupComponent>(trampolineEntity);
	AddComponent<ModelComponent>(trampolineEntity, trampolineID);
	AddComponent<TransformComponent>(trampolineEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(trampolineEntity);
	LuaMain::GetScriptManager()->AddScript(trampolineEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(trampolineEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(trampolineEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void TestScene::CreateMissilePickup(DirectX::SimpleMath::Vector3 position)
{
	u32 missileID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/missile.glb");

	entity missileEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(missileEntity);
	bc.type = BarrelComponent::Type::Missile;
	bc.maximumAmmoCapacityForType = 5;
	bc.ammoPerPickup = 1;
	AddComponent<PickupComponent>(missileEntity);
	AddComponent<ModelComponent>(missileEntity, missileID);
	AddComponent<TransformComponent>(missileEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	AddComponent<ShadowReceiverComponent>(missileEntity);
	LuaMain::GetScriptManager()->AddScript(missileEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(missileEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(missileEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void TestScene::CreateGrenadePickup(DirectX::SimpleMath::Vector3 position)
{
	u32 grenadeID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Grenade/Grenade.fbx");

	entity grenadeEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(grenadeEntity);
	bc.type = BarrelComponent::Type::Grenade;
	bc.maximumAmmoCapacityForType = 10;
	bc.ammoPerPickup = 2;
	AddComponent<PickupComponent>(grenadeEntity);
	AddComponent<ModelComponent>(grenadeEntity, grenadeID);
	AddComponent<TransformComponent>(grenadeEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	AddComponent<ShadowReceiverComponent>(grenadeEntity);
	LuaMain::GetScriptManager()->AddScript(grenadeEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(grenadeEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(grenadeEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}
