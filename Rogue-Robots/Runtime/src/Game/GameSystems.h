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
			stc.SetPosition(stc.GetPosition() + DirectX::SimpleMath::Vector3(0.2f, 1.6f, 0.f));
			slc.dirty = true;

			auto up = ptc.worldMatrix.Up();
			up.Normalize();
			
			auto& pcc = DOG::EntityManager::Get().GetComponent<PlayerControllerComponent>(slc.owningPlayer);
			if (pcc.cameraEntity == DOG::NULL_ENTITY) 
				return;

			auto& playerCameraTransform = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(pcc.cameraEntity);
			
			auto pos = stc.GetPosition();
			auto forward = playerCameraTransform.GetForward();
			
			slc.direction = forward;
			cc.viewMatrix = DirectX::XMMatrixLookToLH(pos, forward, up);
		}
	}
};

class PickupItemInteractionSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	#define REQUIRED_DISTANCE_DELTA 2.0f
	#define REQUIRED_DOT_DELTA -0.90f
public:
	SYSTEM_CLASS(DOG::ThisPlayer, DOG::TransformComponent, PlayerControllerComponent);
	ON_EARLY_UPDATE_ID(DOG::ThisPlayer, DOG::TransformComponent, PlayerControllerComponent);

	void OnEarlyUpdate(DOG::entity player, DOG::ThisPlayer&, DOG::TransformComponent& ptc, PlayerControllerComponent& pcc)
	{
		auto& mgr = DOG::EntityManager::Get();
		auto playerPosition = ptc.GetPosition();
		DOG::entity closestPickup = DOG::NULL_ENTITY;
		float closestDistance = FLT_MAX;

		mgr.Collect<PickupComponent, DOG::TransformComponent>().Do([&closestDistance, &closestPickup, &playerPosition](DOG::entity pickUp, PickupComponent&, DOG::TransformComponent& tc)
			{
				float distanceToPickup = Vector3::Distance(playerPosition, tc.GetPosition());
				if (distanceToPickup < closestDistance)
				{
					closestDistance = distanceToPickup;
					if (closestDistance < REQUIRED_DISTANCE_DELTA)
					{
						closestPickup = pickUp;
					}
				}
			});
		//If we are not near enough or no items exist:
		if (closestPickup == DOG::NULL_ENTITY)
			return;

		auto& tc = mgr.GetComponent<DOG::TransformComponent>(closestPickup);
		auto cameraForward = mgr.GetComponent<DOG::TransformComponent>(pcc.cameraEntity).GetForward();

		// Camera offset
		const auto cOffset = Vector3(0, 0.4f, 0);

		Vector3 pickUpToPlayerDirection = (playerPosition+cOffset) - tc.GetPosition();
		pickUpToPlayerDirection.Normalize();

		float dot = cameraForward.Dot(pickUpToPlayerDirection);
		bool isLookingAtItem = dot < REQUIRED_DOT_DELTA;

		if (!isLookingAtItem)
			return;

		//Checks are done, and this pickup is now considered eligible for being picked up by the player:
		
		//Add the component that highlights it as an eligible item, e.g. for rendering the item name text:
		mgr.AddComponent<EligibleForPickupComponent>(closestPickup).player = player;
		
		//Check if the player has performed an interaction query:
		if (mgr.HasComponent<InteractionQueryComponent>(player))
		{
			//If so we now need to remove the interaction query and have the item lerp to the player:
			mgr.RemoveComponent<InteractionQueryComponent>(player);
			//But only if it is not currently lerping towards the player:
			if (!mgr.HasComponent<LerpToPlayerComponent>(closestPickup))
			{
				mgr.AddComponent<LerpToPlayerComponent>(closestPickup).player = player;
				auto& plac = mgr.GetComponent<DOG::PickupLerpAnimateComponent>(closestPickup);
				plac.origin = tc.GetPosition();
				plac.target = playerPosition;
			}
		}
	}
};

class MVPRenderPickupItemUIText : public DOG::ISystem
{
public:
	SYSTEM_CLASS(PickupComponent, EligibleForPickupComponent);
	ON_UPDATE(PickupComponent, EligibleForPickupComponent);

	void OnUpdate(PickupComponent& pc, EligibleForPickupComponent& efpg)
	{
		//Do not render other players' eligible pickup item names.
		if (efpg.player != DOG::GetPlayer())
			return;

		ImVec2 size;
		size.x = 285;
		size.y = 300;

		auto r = DOG::Window::GetWindowRect();
		ImVec2 pos;
		constexpr float xOffset = -500.0f;
		constexpr float yOffset = 400.0f;
		pos.x = r.right - size.x + xOffset;
		pos.y = r.top + yOffset;

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Pickup item text", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::SetWindowFontScale(1.3f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));
			ImGui::Text("[E]");
			ImGui::PopStyleColor(1);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
			ImGui::SameLine();
			ImGui::Text("Pick up");
			ImGui::Separator();
			ImGui::SetWindowFontScale(1.7f);
			ImGui::Text(pc.itemName);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}
};

class CleanupItemInteractionSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(EligibleForPickupComponent);
	ON_LATE_UPDATE_ID(EligibleForPickupComponent);

	void OnLateUpdate(DOG::entity pickup, EligibleForPickupComponent&)
	{
		//For now, we simply remove any such item components:
		DOG::EntityManager::Get().RemoveComponent<EligibleForPickupComponent>(pickup);
	}
};

class CleanupPlayerStateSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer);
	ON_LATE_UPDATE_ID(DOG::ThisPlayer);

	void OnLateUpdate(DOG::entity player, DOG::ThisPlayer&)
	{
		//System for checking various player states, removing them, etc...
		if (DOG::EntityManager::Get().HasComponent<InteractionQueryComponent>(player))
			DOG::EntityManager::Get().RemoveComponent<InteractionQueryComponent>(player);
	}
};

class MVPRenderAmmunitionTextSystem : public DOG::ISystem
{
#define INFINITY_EQUIVALENT 999'999
public:
	SYSTEM_CLASS(DOG::ThisPlayer, BarrelComponent);
	ON_UPDATE_ID(DOG::ThisPlayer, BarrelComponent);

	void OnUpdate(DOG::entity player, DOG::ThisPlayer, BarrelComponent& bc)
	{
		std::string barrelType;

		if (DOG::EntityManager::Get().HasComponent<MagazineModificationComponent>(player))
		{
			auto type = DOG::EntityManager::Get().GetComponent<MagazineModificationComponent>(player).type;
			switch (type)
			{
			case MagazineModificationComponent::Type::Frost:
			{
				barrelType = "Frost ";
				break;
			}
			}
		}

		switch (bc.type)
		{
		case BarrelComponent::Type::Missile:
		{
			barrelType += "Missiles";
			break;
		}
		case BarrelComponent::Type::Grenade:
		{
			barrelType += "Grenades";
			break;
		}
		case BarrelComponent::Type::Bullet:
		{
			barrelType += "Bullets";
			break;
		}
		}
		if (barrelType.empty())
			return;

		std::string ammoText = std::to_string(bc.currentAmmoCount) + std::string(" / "); 
		bc.maximumAmmoCapacityForType == INFINITY_EQUIVALENT ? ammoText += "INF." : ammoText += std::to_string(bc.maximumAmmoCapacityForType);

		ImVec2 size;
		size.x = 280;
		size.y = 100;

		auto r = DOG::Window::GetWindowRect();
		ImVec2 pos;
		constexpr float xOffset = -60.0f;
		constexpr float yOffset = -170.0f;
		pos.x = r.right - size.x + xOffset;
		pos.y = r.bottom + yOffset;

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Ammo text", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
			ImGui::SetWindowFontScale(2.0f);
			ImGui::Text(barrelType.c_str());
			ImGui::Separator();
			ImGui::Text(ammoText.c_str());
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
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

class PlayerJumpRefreshSystem : public DOG::ISystem
{
	using HasEnteredCollisionComponent = DOG::HasEnteredCollisionComponent;
	using EntityManager = DOG::EntityManager;
	using Entity = DOG::entity;

public:
	SYSTEM_CLASS(PlayerControllerComponent, HasEnteredCollisionComponent);
	ON_UPDATE(PlayerControllerComponent, HasEnteredCollisionComponent);

	void OnUpdate(PlayerControllerComponent& playerController, HasEnteredCollisionComponent& colComp)
	{
		for (auto idx = 0u; auto colEntity: colComp.entities)
		{
			if (idx++ == colComp.entitiesCount) break;

			if (EntityManager::Get().HasComponent<DOG::ModularBlockComponent>(colEntity))
			{
				playerController.jumping = false;
			}
		}
	}
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

class MVPRenderReloadHintTextSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer, BarrelComponent);
	ON_UPDATE(DOG::ThisPlayer, BarrelComponent);

	void OnUpdate(DOG::ThisPlayer, BarrelComponent& bc)
	{
		if (bc.type != BarrelComponent::Type::Bullet)
			return;

		if (!(bc.currentAmmoCount <= 3))
			return;

		ImVec2 size;
		size.x = 240;
		size.y = 100;

		auto r = DOG::Window::GetWindowRect();
		ImVec2 pos;
		constexpr const float xOffset = 50.0f;
		const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
		const float centerYOfScreen = (float)(abs(r.bottom - r.top)) * 0.5f;
		pos.x = r.left + centerXOfScreen + xOffset;
		pos.y = r.top + centerYOfScreen;

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Reload text", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));
			ImGui::SetWindowFontScale(1.4f);
			ImGui::Text("[R] Reload");
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}
};

class ScuffedSceneGraphSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(ParentComponent, DOG::TransformComponent);
	ON_UPDATE_ID(ParentComponent, DOG::TransformComponent);

	void OnUpdate(DOG::entity e, ParentComponent& local, DOG::TransformComponent& world)
	{
		if (DOG::EntityManager::Get().Exists(local.parent) && DOG::EntityManager::Get().HasComponent<DOG::TransformComponent>(local.parent))
		{
			auto& parentWorld = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(local.parent);
			world.worldMatrix = local.localTransform * parentWorld.worldMatrix;
		}
		else
		{
			DOG::EntityManager::Get().DeferredEntityDestruction(e);
		}
	}
};
