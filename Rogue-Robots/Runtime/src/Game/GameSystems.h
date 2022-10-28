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
			stc.SetPosition(stc.GetPosition() + DirectX::SimpleMath::Vector3(-0.2f, 0.6f, 0.f));
			slc.dirty = true;

			auto up = ptc.worldMatrix.Up();
			up.Normalize();
			
			auto& pcc = DOG::EntityManager::Get().GetComponent<PlayerControllerComponent>(slc.owningPlayer);
			if (!pcc.cameraEntity) 
				return;

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
	inline static constexpr Vector3 s_globUp = Vector3(0, 1, 0);

private:
	Entity CreateDebugCamera(Entity e) noexcept;
	Entity CreatePlayerCameraEntity(Entity player) noexcept;

	Vector3 GetNewForward(PlayerControllerComponent& player)  const noexcept;

	Vector3 GetMoveTowards(const InputController& input, Vector3 forward, Vector3 right) const noexcept;

	void MoveDebugCamera(Entity e, Vector3 moveTowards, Vector3 forward, Vector3 right, f32 speed, const InputController& input) noexcept;

	void MovePlayer(Entity e, PlayerControllerComponent& player, Vector3 moveTowards, Vector3 forward,
		RigidbodyComponent& rb, f32 speed, InputController& input);
};
//Quick and dirty flashlight toggle system for MVP
class MVPFlashlightStateSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::SpotLightComponent);
	ON_UPDATE(DOG::SpotLightComponent);

	void OnUpdate(DOG::SpotLightComponent& slc)
	{
		auto player = slc.owningPlayer;
		if (player == DOG::NULL_ENTITY)
			return;

		auto flashlightIsTurnedOn = DOG::EntityManager::Get().GetComponent<InputController>(player).flashlight;

		if (flashlightIsTurnedOn)
			slc.strength = 0.6f;
		else
			slc.strength = 0.0f;
	}
};
class MVPPickupItemInteractionSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	#define REQUIRED_DISTANCE_DELTA 3.0f
	#define REQUIRED_DOT_DELTA -0.25f
public:
	SYSTEM_CLASS(DOG::ThisPlayer, DOG::TransformComponent);
	ON_UPDATE_ID(DOG::ThisPlayer, DOG::TransformComponent);

	void OnUpdate(DOG::entity player, DOG::ThisPlayer&, DOG::TransformComponent& ptc)
	{
		auto& mgr = DOG::EntityManager::Get();
		auto playerPosition = ptc.GetPosition();
		DOG::entity closestPickup = DOG::NULL_ENTITY;
		float closestDistance = FLT_MAX;
		Vector3 pickUpToPlayerDirection;

		mgr.Collect<PickupComponent, DOG::TransformComponent>().Do([&](DOG::entity entityID, PickupComponent&, DOG::TransformComponent& tc)
			{
				if (closestPickup == DOG::NULL_ENTITY)
					closestPickup = entityID;

				float distance = Vector3::Distance(playerPosition, tc.GetPosition());
				if (distance < closestDistance)
				{
					closestDistance = distance; 
					closestPickup = entityID; 
				}
			});
		if (closestPickup == DOG::NULL_ENTITY)
			return;

		auto& tc = mgr.GetComponent<DOG::TransformComponent>(closestPickup);
		pickUpToPlayerDirection = playerPosition - tc.GetPosition();
		pickUpToPlayerDirection.Normalize();

		float dot = ptc.GetForward().Dot(pickUpToPlayerDirection);
		bool isLookingAtItem = dot < REQUIRED_DOT_DELTA;
		bool isCloseEnoughToItem = closestDistance < REQUIRED_DISTANCE_DELTA;
		if (isLookingAtItem && isCloseEnoughToItem)
		{
			if (mgr.HasComponent<ActiveItemComponent>(closestPickup))
			{
				//Pickup is an active item entity, so:
				if (mgr.HasComponent<EligibleActiveItemComponent>(player))
				{
					mgr.RemoveComponent<EligibleActiveItemComponent>(player);
				}
				auto& eaic = mgr.AddComponent<EligibleActiveItemComponent>(player);
				eaic.activeItemEntity = closestPickup;
				eaic.type = mgr.GetComponent<ActiveItemComponent>(closestPickup).type;
			}
		}
		else
		{
			if (mgr.HasComponent<EligibleActiveItemComponent>(player))
			{
				mgr.RemoveComponent<EligibleActiveItemComponent>(player);
			}
		}
	}
};

class MVPRenderPickupItemUIText : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer);
	ON_UPDATE_ID(DOG::ThisPlayer);

	void OnUpdate(DOG::entity playerID, DOG::ThisPlayer&)
	{
		if (DOG::EntityManager::Get().HasComponent<EligibleActiveItemComponent>(playerID))
		{
			//ImVec2 size;
			//size.x = 280;
			//size.y = 300;
			//
			////auto r = Window::GetWindowRect();
			//ImVec2 pos;
			//pos.x = r.right - size.x - 20.0f;
			//pos.y = r.top + 50.0f;
			//
			//ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			//ImGui::SetNextWindowPos(pos);
			//ImGui::SetNextWindowSize(size);
			//if (ImGui::Begin("KeyBindings", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
			//{
			//	if (ImGui::BeginTable("KeyBindings", 2))
			//	{
			//		for (auto& [key, action] : m_kayBindingDescriptions)
			//		{
			//			ImGui::TableNextRow();
			//			ImGui::TableSetColumnIndex(0);
			//			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
			//			ImGui::Text(action.c_str());
			//			ImGui::TableSetColumnIndex(1);
			//			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));
			//			ImGui::Text(key.c_str());
			//			ImGui::PopStyleColor(2);
			//		}
			//		ImGui::EndTable();
			//	}
			//}
			//ImGui::End();
			//ImGui::PopStyleColor();
		}
	}
};
