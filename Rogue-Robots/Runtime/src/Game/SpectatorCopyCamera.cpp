#include "SpectatorCopyCamera.h"

using namespace DOG;

void SpectatorCopyCamera::CollectAndUpdate()
{
	EntityManager::Get().Collect<PlayerControllerComponent, SpectatorComponent>().Do([](entity e, PlayerControllerComponent& player, SpectatorComponent&)
		{
			OnUpdate(e, player);
		});
}

void SpectatorCopyCamera::OnUpdate(entity e, PlayerControllerComponent& player)
{
	auto& mgr = EntityManager::Get();

	// The player might be spectating, and if so the camera should be updated based on the spectated players' camera stats.
	assert(mgr.HasComponent<SpectatorComponent>(e));
	assert(!mgr.HasComponent<PlayerAliveComponent>(e));

	entity playerBeingSpectated = mgr.GetComponent<SpectatorComponent>(e).playerBeingSpectated;
	auto& spectatedPlayerControllerComponent = mgr.GetComponent<PlayerControllerComponent>(playerBeingSpectated);
	auto& spectatedCameraComponent = mgr.GetComponent<CameraComponent>(spectatedPlayerControllerComponent.cameraEntity);
	auto& spectatedCameraTransform = mgr.GetComponent<TransformComponent>(spectatedPlayerControllerComponent.cameraEntity);
	/*DOG::EntityManager::Get().Collect<ModelComponent, ChildToBoneComponent>().Do([&](entity gunModelNotFPS, ModelComponent&, ChildToBoneComponent& childToBone)
		{
			if (childToBone.boneParent == playerBeingSpectated)
			{
				if (EntityManager::Get().HasComponent<PlayerAliveComponent>(playerBeingSpectated) &&
					EntityManager::Get().HasComponent<DontDraw>(gunModelNotFPS) &&
					EntityManager::Get().HasComponent<ScriptComponent>(playerBeingSpectated))
				{
					auto scriptData = LuaMain::GetScriptManager()->GetScript(playerBeingSpectated, "Gun.lua");
					LuaTable tab(scriptData.scriptTable, true);
					auto ge = tab.GetTableFromTable("gunEntity");
		
					int gunID = ge.GetIntFromTable("entityID");
					int barrelID = tab.GetIntFromTable("barrelEntityID");
					int miscID = tab.GetIntFromTable("miscEntityID");
					int magazineID = tab.GetIntFromTable("magazineEntityID");
		
					EntityManager::Get().RemoveComponentIfExists<DontDraw>(gunID);
					EntityManager::Get().RemoveComponentIfExists<DontDraw>(barrelID);
					EntityManager::Get().RemoveComponentIfExists<DontDraw>(miscID);
					EntityManager::Get().RemoveComponentIfExists<DontDraw>(magazineID);
				}
			}
		});*/
	//The player is dead and so MUST have a debug camera (should be spectator camera) assigned, so:
	entity spectatorCamera = player.spectatorCamera;
	auto& spectatorCameraComponent = mgr.GetComponent<CameraComponent>(spectatorCamera);
	auto& spectatorCameraTransform = mgr.GetComponent<TransformComponent>(spectatorCamera);

	//We now simply set them equal:
	spectatorCameraComponent.viewMatrix = spectatedCameraComponent.viewMatrix;
	spectatorCameraComponent.projMatrix = spectatedCameraComponent.projMatrix;
	spectatorCameraTransform.worldMatrix = spectatedCameraTransform.worldMatrix;

	spectatorCameraComponent.isMainCamera = true;
	mgr.GetComponent<CameraComponent>(player.cameraEntity).isMainCamera = false;
	if (mgr.Exists(player.debugCamera))
	{
		mgr.GetComponent<CameraComponent>(player.debugCamera).isMainCamera = false;
	}
}
