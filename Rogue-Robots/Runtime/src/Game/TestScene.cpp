#include "TestScene.h"
#include "GameComponent.h"
#include "../Network/Network.h"
#include "PrefabInstantiatorFunctions.h"
#include "ItemManager/ItemManager.h"
#include "EntitesTypes.h"

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
	u32 isoSphereID = am.LoadModelAsset("Assets/Models/Temporary_Assets/iso_sphere.glb");

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


	// Create a test particle system 
	{
		entity particleEntity = CreateEntity();
		AddComponent<TransformComponent>(particleEntity, Vector3(31.0f, 70.0f, 106.0f));
		auto& system = AddComponent<ParticleEmitterComponent>(particleEntity);
		system = {
			.spawnRate = 128.f,
			.particleLifetime = .5f,
		};

		entity particleEntity2 = CreateEntity();
		AddComponent<TransformComponent>(particleEntity2, Vector3(28.0f, 70.0f, 106.0f));
		auto& system2 = AddComponent<ParticleEmitterComponent>(particleEntity2);
		system2 = {
			.spawnRate = 128.f,
			.particleLifetime = .5f,
		};

		entity particleEntity3 = CreateEntity();
		AddComponent<TransformComponent>(particleEntity3, Vector3(34.0f, 70.0f, 106.0f));
		auto& system3 = AddComponent<ParticleEmitterComponent>(particleEntity3);
		system3 = {
			.spawnRate = 128.f,
			.particleLifetime = .5f,
		};
	}

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
	isoSphereLight.radius = 30;
	isoSphereLight.handle = LightManager::Get().AddPointLight(
		PointLightDesc
		{
			.position = s_entityManager.GetComponent<TransformComponent>(isoSphereEntity).GetPosition(),
			.radius = isoSphereLight.radius,
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
	CreateMissilePickup(Vector3(29.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(32.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(35.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(38.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(41.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(44.0f, 6.0f, 17.0f));
	CreateMissilePickup(Vector3(47.0f, 6.0f, 17.0f));

	CreateGrenadePickup(Vector3(50.0f, 6.0f, 17.0f));

	CreateMaxHealthBoostPickup(Vector3(53.0f, 6.0f, 17.0f));
	CreateMaxHealthBoostPickup(Vector3(56.0f, 6.0f, 17.0f));

	CreateFrostModificationPickup(Vector3(50.0f, 6.0f, 14.0f));
	CreateFrostModificationPickup(Vector3(47.0f, 6.0f, 14.0f));

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
			pdesc.radius = 15.f;
			auto& plc = AddComponent<PointLightComponent>(e);
			plc.handle = LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::Never);
			plc.color = pdesc.color;
			plc.strength = pdesc.strength;
			plc.radius = pdesc.radius;



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
	AddComponent<PointLightComponent>(movingPointLight, pointLight, Vector3(1.f, 1.f, 0.f), 5.f, 10.0f);
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


	AddEntity(SpawnLaserBlob(TransformComponent(Vector3(20, 7, 20)), NULL_ENTITY));


	ItemManager::Get().CreateItem(EntityTypes::LaserBarrel, Vector3(30, 7, 20));
}

void TestScene::CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 trampolineNetworkID = 0u;
	u32 trampolineID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Trampoline.glb");

	entity trampolineEntity = CreateEntity();
	AddComponent<ActiveItemComponent>(trampolineEntity).type = ActiveItemComponent::Type::Trampoline;
	AddComponent<PickupComponent>(trampolineEntity).itemName = "Trampoline";
	AddComponent<ModelComponent>(trampolineEntity, trampolineID);
	AddComponent<TransformComponent>(trampolineEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(trampolineEntity);
	auto& ni = AddComponent<NetworkId>(trampolineEntity);
	ni.entityTypeId = EntityTypes::Trampoline;
	ni.id = trampolineNetworkID++;

	LuaMain::GetScriptManager()->AddScript(trampolineEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(trampolineEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(trampolineEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void TestScene::CreateMissilePickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 missileNetworkID = 0u;
	u32 missileID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Missile/missile.glb");

	entity missileEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(missileEntity);
	bc.type = BarrelComponent::Type::Missile;
	bc.maximumAmmoCapacityForType = 5;
	bc.ammoPerPickup = 1;
	AddComponent<PickupComponent>(missileEntity).itemName = "Homing missile";
	AddComponent<ModelComponent>(missileEntity, missileID);
	AddComponent<TransformComponent>(missileEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	AddComponent<ShadowReceiverComponent>(missileEntity);
	auto& ni = AddComponent<NetworkId>(missileEntity);
	ni.entityTypeId = EntityTypes::MissileBarrel;
	ni.id = missileNetworkID++;

	LuaMain::GetScriptManager()->AddScript(missileEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(missileEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(missileEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void TestScene::CreateGrenadePickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 grenadeNetworkID = 0u;

	u32 grenadeID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Grenade/Grenade.glb");

	entity grenadeEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(grenadeEntity);
	bc.type = BarrelComponent::Type::Grenade;
	bc.maximumAmmoCapacityForType = 10;
	bc.ammoPerPickup = 2;
	AddComponent<PickupComponent>(grenadeEntity).itemName = "Grenade";
	AddComponent<ModelComponent>(grenadeEntity, grenadeID);
	AddComponent<TransformComponent>(grenadeEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<ShadowReceiverComponent>(grenadeEntity);
	auto& ni = AddComponent<NetworkId>(grenadeEntity);
	ni.entityTypeId = EntityTypes::GrenadeBarrel;
	ni.id = grenadeNetworkID++;

	LuaMain::GetScriptManager()->AddScript(grenadeEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(grenadeEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(grenadeEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void TestScene::CreateMaxHealthBoostPickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 healtBoostNetworkdID = 0u;

	u32 healthBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Medkit.glb");

	entity healthBoostEntity = CreateEntity();
	AddComponent<PassiveItemComponent>(healthBoostEntity).type = PassiveItemComponent::Type::MaxHealthBoost;
	AddComponent<PickupComponent>(healthBoostEntity).itemName = "Max HP boost";
	AddComponent<ModelComponent>(healthBoostEntity, healthBoostID);
	AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<ShadowReceiverComponent>(healthBoostEntity);
	auto& ni = AddComponent<NetworkId>(healthBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseMaxHp;
	ni.id = healtBoostNetworkdID++;

	LuaMain::GetScriptManager()->AddScript(healthBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(healthBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(healthBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void TestScene::CreateFrostModificationPickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 frostModNetworkID = 0u;

	u32 frostModID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");

	entity frostModEntity = CreateEntity();
	AddComponent<MagazineModificationComponent>(frostModEntity).type = MagazineModificationComponent::Type::Frost;
	AddComponent<PickupComponent>(frostModEntity).itemName = "Frost modification";
	AddComponent<ModelComponent>(frostModEntity, frostModID);
	AddComponent<TransformComponent>(frostModEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(frostModEntity);
	auto& ni = AddComponent<NetworkId>(frostModEntity);
	ni.entityTypeId = EntityTypes::FrostMagazineModification;
	ni.id = frostModNetworkID++;

	LuaMain::GetScriptManager()->AddScript(frostModEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(frostModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(frostModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}