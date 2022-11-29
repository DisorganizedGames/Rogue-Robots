#include "QueryHelpers.h"
#include <Game\GameComponent.h>

using namespace DOG;

entity GetPlayer() noexcept
{
	entity player = NULL_ENTITY;
	EntityManager::Get().Collect<ThisPlayer>().Do([&player](entity e, ThisPlayer&) { player = e; });
	return player;
}

DOG::entity GetGun() noexcept
{
	entity gun = NULL_ENTITY;
	EntityManager::Get().Collect<ThisPlayerWeapon>().Do([&gun](entity e, ThisPlayerWeapon&) { gun = e; });
	return gun;
}

DOG::entity GetCamera() noexcept
{
	auto& em = EntityManager::Get();
	entity camera = NULL_ENTITY;
	ASSERT(em.Exists(GetPlayer()), "Player don't exist.");
	if (auto controllerComp = em.TryGetComponent<PlayerControllerComponent>(GetPlayer()); controllerComp)
	{
		auto&& findCamera = [](entity potentialCamera) {
			if (EntityManager::Get().Exists(potentialCamera))
			{
				if (auto cameraComponent = EntityManager::Get().TryGetComponent<CameraComponent>(potentialCamera))
					if (cameraComponent->get().isMainCamera) return potentialCamera;
			}
			return NULL_ENTITY;
		};

		camera = findCamera(controllerComp->get().cameraEntity);

		if (camera == NULL_ENTITY)
			camera = findCamera(controllerComp->get().debugCamera);

		if (camera == NULL_ENTITY)
			camera = findCamera(controllerComp->get().spectatorCamera);
	}
	return camera;
}

DOG::entity GetPlayerFPSCamera(DOG::entity player) noexcept
{
	auto& em = EntityManager::Get();
	assert(em.Exists(player) && em.HasComponent<PlayerControllerComponent>(player));
	auto& controllerComp = em.GetComponent<PlayerControllerComponent>(player);
	assert(em.Exists(controllerComp.cameraEntity) && em.HasComponent<CameraComponent>(controllerComp.cameraEntity));
	return controllerComp.cameraEntity;
}
