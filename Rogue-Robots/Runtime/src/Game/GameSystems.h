#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class DoorOpeningSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(DoorComponent, DOG::TransformComponent);
	ON_UPDATE(DoorComponent, DOG::TransformComponent);
	void OnUpdate(DoorComponent& door, DOG::TransformComponent& transform)
	{
		ASSERT(door.roomId != u32(-1), "Door with uninitialized room ID exists");

		if (door.openValue >= 1.f && door.isOpening)
		{
			door.openValue = 1.f;
			door.isOpening = false;
			std::cout << "Door opened!" << "\n";
			return;
		}
		else if (door.openValue >= 1.f) return;
		
		if (door.isOpening)
		{
			// In order to not redundantly store the start position of the door, this calculates the start position given the current lerp value
			// Could be changed if it somehow affects performance
			auto pos = transform.GetPosition();
			auto startY = pos.y - (door.openDisplacementY * sinLerp(door.openValue));

			door.openValue += .5f * static_cast<f32>(DOG::Time::DeltaTime());
		
			transform.SetPosition(Vector3(pos.x, startY + (door.openDisplacementY * sinLerp(door.openValue)), pos.z));

			return;
		}


		// This should be changed to work for any room-clear condition
		bool enemiesAlive = false;
		DOG::EntityManager::Get().Bundle<AgentStatsComponent>().Do([&](AgentStatsComponent& agent)
			{
				if (enemiesAlive)
					return;

				enemiesAlive = (agent.roomId == door.roomId);
			});

		if (!enemiesAlive)
		{
			std::cout << "Door " << door.roomId << " opening\n";
			door.isOpening = true;
		}
	}

	f32 sinLerp(f32 lerper)
	{
		return (std::sinf(lerper*DirectX::XM_PI - DirectX::XM_PIDIV2) / 2.f + .5f);
	}
};

class MVPFlashlightMoveSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::SpotLightComponent, DOG::CameraComponent, DOG::TransformComponent);
	ON_UPDATE(DOG::SpotLightComponent, DOG::CameraComponent, DOG::TransformComponent);

	void OnUpdate(DOG::SpotLightComponent& slc, DOG::CameraComponent& cc, DOG::TransformComponent& stc)
	{
		if (slc.owningPlayer != DOG::NULL_ENTITY)
		{
			auto& ptc = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(slc.owningPlayer);
			stc.worldMatrix = ptc.worldMatrix;
			stc.SetPosition(stc.GetPosition() + DirectX::SimpleMath::Vector3(0.2f, 0.2f, 0.0f));
			slc.direction = ptc.GetForward();
			slc.dirty = true;

			auto up = ptc.worldMatrix.Up();
			up.Normalize();

			cc.viewMatrix = DirectX::XMMatrixLookAtLH
			(
				{ stc.GetPosition().x, stc.GetPosition().y, stc.GetPosition().z },
				{ stc.GetPosition().x + stc.GetForward().x, stc.GetPosition().y + stc.GetForward().y, stc.GetPosition().z + stc.GetForward().z },
				{ up.x, up.y, up.z }
			);
		}
	}
};