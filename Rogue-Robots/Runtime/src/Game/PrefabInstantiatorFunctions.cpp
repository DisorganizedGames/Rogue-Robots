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
	playerModels[0] = am.LoadModelAsset("Assets/Models/Players/Test/Red/player_red.gltf");
	playerModels[1] = am.LoadModelAsset("Assets/Models/Temporary_Assets/green_cube.glb", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	playerModels[2] = am.LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");
	playerModels[3] = am.LoadModelAsset("Assets/Models/Temporary_Assets/magenta_cube.glb");
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
		em.AddComponent<ModelComponent>(playerI, playerModels[i]);
		em.AddComponent<CapsuleColliderComponent>(playerI, playerI, 0.25f, 0.8f, true, 75.f);
		auto& rb = em.AddComponent<RigidbodyComponent>(playerI, playerI);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = true;

		em.AddComponent<PlayerStatsComponent>(playerI);
		em.AddComponent<PlayerControllerComponent>(playerI);
		em.AddComponent<NetworkPlayerComponent>(playerI).playerId = static_cast<i8>(i);
		em.AddComponent<InputController>(playerI);
		em.AddComponent<ShadowReceiverComponent>(playerI);
		em.AddComponent<PlayerAliveComponent>(playerI);
		scriptManager->AddScript(playerI, "Gun.lua");
		scriptManager->AddScript(playerI, "PassiveItemSystem.lua");
		scriptManager->AddScript(playerI, "ActiveItemSystem.lua");

		if (i == 0) // Only for this player
		{
			em.AddComponent<ThisPlayer>(playerI);
			em.AddComponent<AudioListenerComponent>(playerI);
			em.AddComponent<AnimationComponent>(playerI);
			auto& ac = em.GetComponent<AnimationComponent>(playerI);
			ac.animatorID = i;
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
		dd.cutoffAngle = 20.0f;

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
