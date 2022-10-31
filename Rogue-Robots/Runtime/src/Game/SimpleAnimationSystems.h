#pragma once
#include <DOGEngine.h>

class LerpAnimationSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(DOG::LerpAnimateComponent, DOG::TransformComponent);
	ON_UPDATE_ID(DOG::LerpAnimateComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity entityID, DOG::LerpAnimateComponent& animator, DOG::TransformComponent& transform)
	{
		if (abs(animator.loops) > 0)
		{
			f64 dt = DOG::Time::DeltaTime<>();
			animator.t += dt * animator.scale;
			f64 t01 = std::clamp(animator.t, 0.0, 1.0);
			Vector3 pos = Vector3::Lerp(animator.origin, animator.target, static_cast<float>(t01));
			transform.SetPosition(pos);
			if (DOG::EntityManager::Get().HasComponent<DOG::DirtyComponent>(entityID))
				DOG::EntityManager::Get().GetComponent<DOG::DirtyComponent>(entityID).dirtyBitSet[DOG::DirtyComponent::positionChanged] = true;
			else
				DOG::EntityManager::Get().AddComponent<DOG::DirtyComponent>(entityID).dirtyBitSet[DOG::DirtyComponent::positionChanged] = true;
			if (animator.t < t01)
			{
				animator.scale = abs(animator.scale);
				animator.t = t01 - animator.t;
				animator.loops--;
			}
			else if (animator.t > t01)
			{
				animator.scale = -abs(animator.scale);
				animator.t = 1.0 + t01 - animator.t;
				animator.loops--;
			}
		}
		if (animator.loops == 0)
		{
			DOG::EntityManager::Get().RemoveComponent<DOG::LerpAnimateComponent>(entityID);
		}
	}
};

class PickupLerpAnimationSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	#define SPEED_MULTIPLER 0.25f
	#define DISTANCE_THRESHOLD 0.60f
public:
	SYSTEM_CLASS(DOG::PickupLerpAnimateComponent, DOG::TransformComponent);
	ON_UPDATE(DOG::PickupLerpAnimateComponent, DOG::TransformComponent);
	void OnUpdate(DOG::PickupLerpAnimateComponent& animator, DOG::TransformComponent& transform)
	{
		animator.currentOrigin = Lerp(animator.currentOrigin, animator.baseTarget, (float)DOG::Time::DeltaTime() * SPEED_MULTIPLER);
		transform.SetPosition({ transform.GetPosition().x, animator.currentOrigin, transform.GetPosition().z });

		if (abs(transform.GetPosition().y - animator.baseTarget) < DISTANCE_THRESHOLD)
		{
			std::swap(animator.baseTarget,animator.baseOrigin);
		}

		transform.RotateW({(float)DOG::Time::DeltaTime(), (float)DOG::Time::DeltaTime(), 0.0f});
	}

	float Lerp(float a, float b, float t)
	{
		return (1.0f - t) * a + b * t;
	}
};

class PickUpTranslateToPlayerSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	#define PICKUP_SPEED 17.0f
	#define PICKUP_RADIUS 1.3f
	#define PICKUP_Y_OFFSET 1.0f
public:
	SYSTEM_CLASS(PickedUpItemComponent, DOG::TransformComponent, DOG::PickupLerpAnimateComponent);
	ON_UPDATE_ID(PickedUpItemComponent, DOG::TransformComponent, DOG::PickupLerpAnimateComponent);

	void OnUpdate(DOG::entity itemEntity, PickedUpItemComponent&, DOG::TransformComponent& tc, DOG::PickupLerpAnimateComponent& plac)
	{
		auto& mgr = DOG::EntityManager::Get();
		auto newPos = Vector3::Lerp(plac.origin, plac.target, (float)DOG::Time::DeltaTime() * PICKUP_SPEED);
		tc.SetPosition(newPos);
		mgr.Collect<DOG::ThisPlayer, DOG::TransformComponent>().Do([&](DOG::entity player, DOG::ThisPlayer&, DOG::TransformComponent& ptc)
			{
				plac.origin = tc.GetPosition();
				plac.target = { ptc.GetPosition().x, ptc.GetPosition().y - PICKUP_Y_OFFSET, ptc.GetPosition().z };
				if (Vector3::Distance(tc.GetPosition(), ptc.GetPosition()) < PICKUP_RADIUS)
				{
					std::string luaEventName = std::string("ItemPickup") + std::to_string(player);
					DOG::LuaMain::GetEventSystem()->InvokeEvent(luaEventName, itemEntity);
					mgr.DeferredEntityDestruction(itemEntity);
				}
			});
	}
};

class LerpColorSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Vector4 = DirectX::SimpleMath::Vector4;
public:
	SYSTEM_CLASS(DOG::LerpColorComponent, DOG::SubmeshRenderer);
	ON_UPDATE_ID(DOG::LerpColorComponent, DOG::SubmeshRenderer);
	void OnUpdate(DOG::entity entityID, DOG::LerpColorComponent& animator, DOG::SubmeshRenderer& mat)
	{
		if (abs(animator.loops) > 0)
		{
			f64 dt = DOG::Time::DeltaTime<>();
			animator.t += dt * animator.scale;
			f64 t01 = std::clamp(animator.t, 0.0, 1.0);
			Vector3 color = Vector3::Lerp(animator.origin, animator.target, static_cast<float>(t01));
			mat.materialDesc.albedoFactor = Vector4(color.x, color.y, color.z, 1);
			mat.dirty = true;
			if (animator.t < t01)
			{
				animator.scale = abs(animator.scale);
				animator.t = t01 - animator.t;
				animator.loops--;
			}
			else if (animator.t > t01)
			{
				animator.scale = -abs(animator.scale);
				animator.t = 1.0 + t01 - animator.t;
				animator.loops--;
			}
		}
		if (animator.loops == 0)
		{
			DOG::EntityManager::Get().RemoveComponent<DOG::LerpColorComponent>(entityID);
		}
	}
};