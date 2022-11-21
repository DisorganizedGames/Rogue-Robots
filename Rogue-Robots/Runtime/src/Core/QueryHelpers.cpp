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
	if (auto controllerComp = em.TryGetComponent<PlayerControllerComponent>(GetPlayer()); controllerComp)
	{

		camera = controllerComp->get().cameraEntity;
		if (auto cameraComponent = em.TryGetComponent<CameraComponent>(camera); cameraComponent)
		{
			if (cameraComponent->get().isMainCamera)
				return camera;
		}

		camera = controllerComp->get().debugCamera;
		if (auto cameraComponent = em.TryGetComponent<CameraComponent>(camera); cameraComponent)
		{
			if (cameraComponent->get().isMainCamera)
				return camera;
		}
	}
	return camera;
}
