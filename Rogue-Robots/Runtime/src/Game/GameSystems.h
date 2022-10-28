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
			stc.SetPosition(stc.GetPosition() + DirectX::SimpleMath::Vector3(0.2f, 0.6f, 0.f));
			slc.dirty = true;

			auto up = ptc.worldMatrix.Up();
			up.Normalize();
			
			auto& pcc = DOG::EntityManager::Get().GetComponent<PlayerControllerComponent>(slc.owningPlayer);
			auto& playerCameraTransform = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(pcc.cameraEntity);
			
			auto pos = stc.GetPosition();
			auto forward = playerCameraTransform.GetForward();
			
			slc.direction = forward;
			cc.viewMatrix = DirectX::XMMatrixLookToLH(pos, forward, up);
		}
	}
};

class PlayerMovementSystem : public DOG::ISystem
{
	using TransformComponent = DOG::TransformComponent;
	using CameraComponent = DOG::CameraComponent;
	using RigidbodyComponent = DOG::RigidbodyComponent;
	using Entity = DOG::entity;

	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;

public:
	SYSTEM_CLASS(PlayerControllerComponent, PlayerStatsComponent, TransformComponent, RigidbodyComponent, InputController);
	ON_EARLY_UPDATE_ID(PlayerControllerComponent, PlayerStatsComponent, TransformComponent, RigidbodyComponent, InputController);

	void OnEarlyUpdate(Entity, PlayerControllerComponent&, PlayerStatsComponent&, TransformComponent&, RigidbodyComponent&, InputController&);

private:
	bool m_useDebug = false;
	Entity m_debugCamera = 0;

	inline static constexpr Vector3 s_globUp = Vector3(0, 1, 0);

private:
	void CreateDebugCamera() noexcept;
	Vector3 GetNewForward(PlayerControllerComponent& player)  const noexcept;

	Vector3 GetMoveTowards(const InputController& input, Vector3 forward, Vector3 right) const noexcept;

	void MoveDebugCamera(Vector3 moveTowards, Vector3 forward, Vector3 right, f32 speed, const InputController& input) noexcept;
};
