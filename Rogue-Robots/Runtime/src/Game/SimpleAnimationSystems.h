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