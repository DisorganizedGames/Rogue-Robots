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
	playerModels[0] = am.LoadModelAsset("Assets/Models/P2/Red/player_red.gltf");
	playerModels[1] = am.LoadModelAsset("Assets/Models/P2/Blue/player_Blue.gltf", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	playerModels[2] = am.LoadModelAsset("Assets/Models/P2/Green/player_Green.gltf");
	playerModels[3] = am.LoadModelAsset("Assets/Models/P2/Yellow/player_yellow.gltf");
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
		em.AddComponent<CapsuleColliderComponent>(playerI, playerI, 0.25f, 1.8f, true, 75.f);
		auto& rb = em.AddComponent<RigidbodyComponent>(playerI, playerI);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = true;

		em.AddComponent<PlayerStatsComponent>(playerI);
		em.AddComponent<PlayerControllerComponent>(playerI);
		em.AddComponent<NetworkPlayerComponent>(playerI).playerId = static_cast<i8>(i);
		em.AddComponent<InputController>(playerI);
		em.AddComponent<PlayerAliveComponent>(playerI);
		auto& bc = em.AddComponent<BarrelComponent>(playerI);
		bc.type = BarrelComponent::Type::Bullet;
		bc.maximumAmmoCapacityForType = 999'999;
		bc.ammoPerPickup = 30;
		bc.currentAmmoCount = 30;
		em.AddComponent<MagazineModificationComponent>(playerI).type = MagazineModificationComponent::Type::None;

		scriptManager->AddScript(playerI, "Gun.lua");
		scriptManager->AddScript(playerI, "PassiveItemSystem.lua");
		scriptManager->AddScript(playerI, "ActiveItemSystem.lua");

		entity modelEntity = em.CreateEntity();

		em.AddComponent<TransformComponent>(modelEntity);
		em.AddComponent<ModelComponent>(modelEntity, playerModels[i]);
		em.AddComponent<AnimationComponent>(modelEntity);
		em.AddComponent<ShadowReceiverComponent>(modelEntity);

		auto& ac = em.GetComponent<AnimationComponent>(modelEntity);
		ac.animatorID = static_cast<i8>(i);

		auto& t = em.AddComponent<ParentComponent>(modelEntity);
		t.parent = playerI;
		t.localTransform.SetPosition({0.0f, -0.9f, 0.0f});

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
