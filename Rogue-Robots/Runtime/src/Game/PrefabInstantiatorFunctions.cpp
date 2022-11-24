#include "PrefabInstantiatorFunctions.h"
#include "GameComponent.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

std::vector<DOG::entity> SpawnPlayers(const Vector3& pos, u8 playerCount, f32 spread)
{
	ASSERT(playerCount > 0, "Need to at least spawn ThisPlayer. I.e. playerCount has to exceed 0");
	
	auto* scriptManager = LuaMain::GetScriptManager();
	//// Add persistent material prefab lua
	//{
	//	entity e = m_entityManager.CreateEntity();
	//	m_entityManager.AddComponent<TransformComponent>(e);
	//	scriptManager->AddScript(e, "MaterialPrefabs.lua");
	//}

	//LuaMain::GetGlobal().
	scriptManager->RunLuaFile("MaterialPrefabs.lua");
	LuaMain::GetGlobal()->GetTable("MaterialPrefabs").CallFunctionOnTable("OnStart");

	auto& am = AssetManager::Get();
	auto& em = EntityManager::Get();
	std::array<u32, 4> playerModels;
	playerModels[0] = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/RedRifle/rifleTestRed.gltf");
	playerModels[1] = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/BlueRifle/rifleTestBlue.gltf", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	playerModels[2] = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/GreenRifle/rifleTestGreen.gltf");
	playerModels[3] = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/YellowRifle/rifleTestYellow.gltf");
	std::vector<entity> players;
	for (auto i = 0; i < playerCount; ++i)
	{
		entity playerI = players.emplace_back(em.CreateEntity());
		Vector3 offset = {
			spread * (i % 2) - (spread / 2.f),
			0,
			spread * (i / 2) - (spread / 2.f),
		};
		em.AddComponent<TransformComponent>(playerI, pos - offset);
		em.AddComponent<CapsuleColliderComponent>(playerI, playerI, 0.25f, 1.25f, true, 75.f);
		auto& rb = em.AddComponent<RigidbodyComponent>(playerI, playerI);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = true;
		rb.setGravityForRigidbody = true;
		//Set the gravity for the player to 2.5g
		rb.gravityForRigidbody = Vector3(0.0f, -25.0f, 0.0f);

		em.AddComponent<PlayerStatsComponent>(playerI);
		em.AddComponent<PlayerControllerComponent>(playerI);
		auto& npc = em.AddComponent<NetworkPlayerComponent>(playerI);
		npc.playerId = static_cast<i8>(i);
		npc.playerName = i == 0 ? "Red" : i == 1 ? "Blue" : i == 2 ? "Green" : "Yellow";
		em.AddComponent<InputController>(playerI);
		em.AddComponent<PlayerAliveComponent>(playerI);
		em.AddComponent<AnimationComponent>(playerI);
		em.AddComponent<AudioComponent>(playerI);
		em.AddComponent<MixamoHeadJointTF>(playerI);

		auto& ac = em.GetComponent<AnimationComponent>(playerI);
		ac.animatorID = static_cast<i8>(i);
		ac.SimpleAdd(static_cast<i8>(MixamoAnimations::Idle));

		auto& bc = em.AddComponent<BarrelComponent>(playerI);
		bc.type = BarrelComponent::Type::Bullet;
		bc.maximumAmmoCapacityForType = 999'999;
		bc.ammoPerPickup = 30;
		bc.currentAmmoCount = 30;

		em.AddComponent<MagazineModificationComponent>(playerI).type = MagazineModificationComponent::Type::None;
		em.AddComponent<MiscComponent>(playerI).type = MiscComponent::Type::Basic;

		entity modelEntity = em.CreateEntity();

		em.AddComponent<TransformComponent>(modelEntity);
		em.AddComponent<ModelComponent>(modelEntity, playerModels[i]);
		em.AddComponent<RigDataComponent>(modelEntity);
		em.AddComponent<ShadowReceiverComponent>(modelEntity);

		auto& rc = em.GetComponent<RigDataComponent>(modelEntity);
		rc.offset = i * MIXAMO_RIG.nJoints;

		auto& t = em.AddComponent<ChildComponent>(modelEntity);
		t.parent = playerI;
		t.localTransform.SetPosition({0.0f, -0.5f, 0.0f});

		if (i == 0) // Only for this player
		{
			em.AddComponent<DontDraw>(modelEntity);
			em.AddComponent<ThisPlayer>(playerI);
			em.AddComponent<AudioListenerComponent>(playerI);
		}
		else
		{
			em.AddComponent<OnlinePlayer>(playerI);
		}

		scriptManager->AddScript(playerI, "Gun.lua");
		scriptManager->AddScript(playerI, "PassiveItemSystem.lua");
		scriptManager->AddScript(playerI, "ActiveItemSystem.lua");
	}
	return players;
}


std::vector<entity> AddFlashlightsToPlayers(const std::vector<entity>& players)
{
	auto& em = EntityManager::Get();
	std::vector<entity> flashlights;
	for (auto i = 0; i < players.size(); ++i)
	{
		auto& playerTransformComponent = em.GetComponent<TransformComponent>(players[i]);

		entity flashLightEntity = em.CreateEntity();
		auto& tc = em.AddComponent<DOG::TransformComponent>(flashLightEntity);
		tc.SetPosition(playerTransformComponent.GetPosition() + DirectX::SimpleMath::Vector3(0.2f, 0.2f, 0.0f));

		auto up = tc.worldMatrix.Up();
		up.Normalize();

		auto& cc = em.AddComponent<DOG::CameraComponent>(flashLightEntity);
		cc.isMainCamera = false;
		cc.viewMatrix = DirectX::XMMatrixLookAtLH
		(
			{ tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z },
			{ tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z },
			{ up.x, up.y, up.z }
		);

		auto dd = DOG::SpotLightDesc();
		dd.color = { 1.0f, 1.0f, 1.0f };
		dd.direction = tc.GetForward();
		dd.strength = 0.6f;
		dd.cutoffAngle = 33.0f;

		auto lh = DOG::LightManager::Get().AddSpotLight(dd, DOG::LightUpdateFrequency::PerFrame);

		auto& slc = em.AddComponent<DOG::SpotLightComponent>(flashLightEntity);
		slc.color = dd.color;
		slc.direction = tc.GetForward();
		slc.strength = dd.strength;
		slc.cutoffAngle = dd.cutoffAngle;
		slc.handle = lh;
		slc.owningPlayer = players[i];

		float fov = ((slc.cutoffAngle + 0.1f) * 2.0f) * DirectX::XM_PI / 180.f;
		cc.projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, 1, 800.f, 0.1f);

#if defined NDEBUG
		em.AddComponent<DOG::ShadowCasterComponent>(flashLightEntity);
#endif
		if (i == 0) // Only for this/main player
			slc.isMainPlayerSpotlight = true;
		else
			slc.isMainPlayerSpotlight = false;

		flashlights.push_back(flashLightEntity);
	}
	return flashlights;
}

entity SpawnTurretProjectile(const DirectX::SimpleMath::Matrix& transform, float speed, float dmg, float lifeTime, DOG::entity turret, DOG::entity owner)
{
	constexpr float radius = 0.1f;
	static bool init = true;
	static SubmeshRenderer projectileModel;
	if (init)
	{
		auto shapeCreator = ShapeCreator(Shape::sphere, 16, 16, radius);
		auto shape = shapeCreator.GetResult();
		MeshDesc mesh;
		mesh.indices = shape->mesh.indices;
		mesh.submeshData = shape->submeshes;
		for (auto& [attr, vert] : shape->mesh.vertexData)
		{
			mesh.vertexDataPerAttribute[attr] = vert;
		}
		projectileModel.mesh = CustomMeshManager::Get().AddMesh(mesh).first;
		projectileModel.materialDesc.emissiveFactor = { 1.6f, 0.8f, 0.002f, 1 };
		projectileModel.materialDesc.albedoFactor = { 0.2f, 0.2f, 0.2f, 1 };
		projectileModel.material = CustomMaterialManager::Get().AddMaterial(projectileModel.materialDesc);
		init = false;
	}

	auto& em = EntityManager::Get();

	entity p = em.CreateEntity();
	if (auto scene = em.TryGetComponent<SceneComponent>(turret); scene) em.AddComponent<SceneComponent>(p, scene->get().scene);

	auto& pTransform = em.AddComponent<TransformComponent>(p);
	pTransform.worldMatrix = transform;
	em.AddComponent<SubmeshRenderer>(p) = projectileModel;

	em.AddComponent<SphereColliderComponent>(p, p, radius, true, 0.1f);
	auto& rb = em.AddComponent<RigidbodyComponent>(p, p);
	rb.continuousCollisionDetection = true;
	rb.linearVelocity = speed * pTransform.GetForward();




	LightHandle pointLight = LightManager::Get().AddPointLight(PointLightDesc(), LightUpdateFrequency::PerFrame);
	em.AddComponent<PointLightComponent>(p, pointLight, Vector3(1, 0.5f, 0.001f), 5.f);


	em.AddComponent<TurretProjectileComponent>(p).maxLifeTime = lifeTime;
	auto& bullet = em.AddComponent<BulletComponent>(p);
	bullet.damage = dmg;
	bullet.playerEntityID = owner;

	return p;
}

DOG::entity SpawnLaserBlob(const DOG::TransformComponent& transform, DOG::entity owner) noexcept
{
	auto& em = EntityManager::Get();
	

	static std::optional<SubmeshRenderer> laserModel = std::nullopt;
	if (!laserModel)
	{
		MaterialDesc matDesc;
		matDesc.emissiveFactor = 6 * Vector4(1.5f, 0.1f, 0.1f, 0);
		matDesc.albedoFactor = { 0.5f, 0, 0, 1 };
		TransformComponent matrix;
		matrix.RotateW({ XM_PIDIV2, 0, 0 }).SetScale(Vector3(0.08f, 0.6f, 0.08f));
		laserModel = CreateSimpleModel(matDesc, ShapeCreator(Shape::prism, 16, 8).GetResult()->mesh, matrix);
	}

	entity laser = em.CreateEntity();
	em.AddComponent<TransformComponent>(laser) = transform;
	em.AddComponent<SubmeshRenderer>(laser) = *laserModel;

	if (em.Exists(owner))
	{
		if (auto scene = em.TryGetComponent<SceneComponent>(owner); scene) em.AddComponent<SceneComponent>(laser, scene->get().scene);
	}



	return laser;
}
