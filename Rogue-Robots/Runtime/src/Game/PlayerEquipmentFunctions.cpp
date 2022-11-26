#include "PlayerEquipmentFunctions.h"
#include "PrefabInstantiatorFunctions.h"
#include "../Core/QueryHelpers.h"

#include <DOGEngine.h>
#include "GameComponent.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;


void ThrowGlowStick(DOG::entity thrower, float speed) noexcept
{
	auto& em = EntityManager::Get();
	assert(em.Exists(thrower) && em.Exists(GetCamera()));
	assert(em.HasComponent<TransformComponent>(GetCamera()));

	entity player = thrower;
	entity stick = SpawnGlowStick(player);

	auto& rb = em.GetComponent<RigidbodyComponent>(stick);
	auto& tr = em.GetComponent<TransformComponent>(stick);

	if (auto playerRb = em.TryGetComponent<RigidbodyComponent>(player))
	{
		// Disable collisions between the glowStic and the player to make the throwing work well.
		PhysicsEngine::SetIgnoreCollisionCheck(rb.rigidbodyHandle, playerRb->get().rigidbodyHandle, true);

		// Enable the collision after one second.
		auto& args = em.AddComponent<DeferredSetIgnoreCollisionCheckComponent>(stick);
		args.countDown = 2.0f;
		args.other = player;
		args.value = false;
	}

	auto& cameraTransform = EntityManager::Get().GetComponent<TransformComponent>(GetCamera());
	Vector3 aimDir = cameraTransform.GetForward();
	Vector3 rotationAxis = cameraTransform.GetRight();
	Vector3 pos = tr.GetPosition();
	pos += tr.GetForward() + tr.GetUp();
	tr.SetPosition(pos);

	rb.angularVelocity = DirectX::XM_2PI * rotationAxis;
	rb.linearVelocity = speed * aimDir;
}
