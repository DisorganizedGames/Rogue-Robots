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
	assert(em.Exists(thrower));
	assert(em.Exists(GetPlayerFPSCamera(thrower)));
	assert(em.HasComponent<TransformComponent>(GetPlayerFPSCamera(thrower)));

	auto& cameraTransform = EntityManager::Get().GetComponent<TransformComponent>(GetPlayerFPSCamera(thrower));

	entity player = thrower;

	TransformComponent stickTransform = cameraTransform;

	// Offset the glow stick spawnPos a bit forward and left
	stickTransform.worldMatrix = Matrix::CreateTranslation(-0.5f * Vector3::Forward - 0.3f * Vector3::Right) * stickTransform.worldMatrix;
	entity stick = SpawnGlowStick(stickTransform, player);

	auto& rb = em.GetComponent<RigidbodyComponent>(stick);

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

	rb.angularVelocity = DirectX::XM_2PI * -cameraTransform.GetRight();
	rb.linearVelocity = speed * cameraTransform.GetForward();

	int counter = 0;
	em.Collect<GlowStickComponent>().Do([&counter](GlowStickComponent&) { counter++; });


	if (counter > GlowStickComponent::globalGlowStickLimit)
	{
		std::vector<std::pair<entity, f32>> glowSticks;
		glowSticks.reserve(GlowStickComponent::globalGlowStickLimit);
		em.Collect<GlowStickComponent>().Do([&glowSticks](entity e, GlowStickComponent& g) { glowSticks.emplace_back(e, g.spawnTime); });

		auto it = std::min_element(glowSticks.begin(), glowSticks.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
		em.DeferredEntityDestruction(it->first);
	}
}
