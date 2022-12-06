#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"
#include "InGameMenu.h"

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
		if (slc.owningPlayer != DOG::NULL_ENTITY && DOG::EntityManager::Get().Exists(slc.owningPlayer))
		{
			slc.dirty = true;

			auto up = stc.worldMatrix.Up();
			up.Normalize();
			
			if (!DOG::EntityManager::Get().HasComponent<PlayerControllerComponent>(slc.owningPlayer))
				return;
			auto& pcc = DOG::EntityManager::Get().GetComponent<PlayerControllerComponent>(slc.owningPlayer);
			if (pcc.cameraEntity == DOG::NULL_ENTITY) 
				return;

			if (!DOG::EntityManager::Get().HasComponent<DOG::ThisPlayer>(slc.owningPlayer))
			{
				slc.isMainPlayerSpotlight = false;
			}
			else
			{
				slc.isMainPlayerSpotlight = true;
			}

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
	SYSTEM_CLASS(DOG::ThisPlayer, PlayerAliveComponent, DOG::TransformComponent, PlayerControllerComponent);
	ON_EARLY_UPDATE_ID(DOG::ThisPlayer, PlayerAliveComponent, DOG::TransformComponent, PlayerControllerComponent);

	void OnEarlyUpdate(DOG::entity player, DOG::ThisPlayer&, PlayerAliveComponent&, DOG::TransformComponent& ptc, PlayerControllerComponent& pcc)
	{
		auto& mgr = DOG::EntityManager::Get();
		auto playerPosition = ptc.GetPosition();
		DOG::entity closestPickup = DOG::NULL_ENTITY;
		float closestDistance = FLT_MAX;

		if (pcc.cameraEntity != DOG::NULL_ENTITY)
		{
			playerPosition = mgr.GetComponent<DOG::TransformComponent>(pcc.cameraEntity).GetPosition();
		}

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
		const auto cOffset = Vector3(0, 0.1f, 0);

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
		if (efpg.player != GetPlayer())
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
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
			ImGui::SameLine();
			ImGui::Text("Pick up");
			ImGui::Separator();
			ImGui::SetWindowFontScale(1.7f);
			ImGui::Text(pc.itemName);
			ImGui::PopStyleColor(1);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
			ImGui::SetWindowFontScale(1.2f);
			ImGui::TextWrapped(GetDescriptiveText(pc.itemName).c_str());
			//ImGui::Text(GetDescriptiveText(pc.itemName).c_str());
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

private:
	std::string GetDescriptiveText(std::string item)
	{
		//Active items
		if (item == "Reviver")
		{
			return "Use to revive a teammate.";
		}
		else if (item == "Syringe")
		{
			return "Use to restore HP.";
		}
		else if (item == "Goal Radar")
		{
			return "Use to see the exit for a short while.";
		}
		else if (item == "Trampoline")
		{
			return "Use this to be the life of the party!";
		}
		else if (item == "Turret")
		{
			return "A trusty friend for when your teammates are not.";
		}
		//Passive items
		else if (item == "JumpBoost")
		{
			return "Increases jump height.";
		}
		else if (item == "Speed Boost")
		{
			return "Increases move speed.";
		}
		else if (item == "Speed Boost X2")
		{
			return "WROOOOOM!";
		}
		else if (item == "Max HP boost")
		{
			return "Increases max health.";
		}
		//Components
		else if (item == "Homing missile")
		{
			return "Shoot rockets!";
		}
		else if (item == "Grenade")
		{
			return "Shoot grenades!";
		}
		else if (item == "Laser")
		{
			return "Zap zap!";
		}
		else if (item == "Frost modification")
		{
			return "Makes enemies slower.";
		}
		else if (item == "Fire modification")
		{
			return "Burn baby burn!";
		}
		else if (item == "Full Auto")
		{
			return "So anyway I started blasting.";
		}
		else if (item == "Charge Shot")
		{
			return "Hold to supercharge!";
		}
		else
		{
			return "";
		}
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

	void OnUpdate(DOG::entity, DOG::ThisPlayer, BarrelComponent& bc)
	{
		if (DOG::UI::Get()->GetActiveUIScene() == InGameMenu::s_sceneID)
		{
			return;
		}
		std::string ammoText = std::to_string(bc.currentAmmoCount) + std::string(" / "); 
		bc.maximumAmmoCapacityForType == INFINITY_EQUIVALENT ? ammoText += "INF." : ammoText += std::to_string(bc.maximumAmmoCapacityForType);

		ImVec2 size;
		size.x = 280;
		size.y = 100;

		auto r = DOG::Window::GetWindowRect();
		ImVec2 pos;
		constexpr float xOffset = 150.0f;
		constexpr float yOffset = -148.0f;
		pos.x = r.left + size.x + xOffset;
		pos.y = r.bottom + yOffset;

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Ammo text", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
			ImGui::SetWindowFontScale(1.6f);
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
	using AnimationComponent = DOG::AnimationComponent;

	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;

public:
	PlayerMovementSystem();

	SYSTEM_CLASS(PlayerControllerComponent, PlayerStatsComponent, TransformComponent, RigidbodyComponent, InputController, AnimationComponent);
	ON_EARLY_UPDATE_ID(PlayerControllerComponent, PlayerStatsComponent, TransformComponent, RigidbodyComponent, InputController);

	void OnEarlyUpdate(Entity, PlayerControllerComponent&, PlayerStatsComponent&, TransformComponent&, RigidbodyComponent&, InputController&);

private:
	inline static constexpr Vector3 s_globUp = Vector3(0, 1, 0);
	u32 m_changeSound = 0;
	f32 m_timeBetween = 0.3f;
	f32 m_timeBeteenTimer = 0.0f;
	std::vector<u32> m_footstepSounds;
	u32 m_jumpSound;

private:

	Entity CreateDebugCamera(Entity e) noexcept;
	Entity CreatePlayerCameraEntity(Entity player) noexcept;

	Vector3 GetNewForward(PlayerControllerComponent& player)  const noexcept;

	Vector3 GetMoveTowards(const InputController& input, Vector3 forward, Vector3 right) const noexcept;

	void MoveDebugCamera(Entity e, Vector3 moveTowards, Vector3 forward, Vector3 right, f32 speed, const InputController& input) noexcept;
	
	void ApplyAnimations(Entity e, const InputController& input);

	void MovePlayer(Entity e, PlayerControllerComponent& player, Vector3 moveTowards, Vector3 forward,
		RigidbodyComponent& rb, f32 speed, f32 jumpSpeed, InputController& input);
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
private:
	u32 m_flashlightTurnOnSound;
	u32 m_flashlightTurnOffSound;

public:

	MVPFlashlightStateSystem()
	{
		m_flashlightTurnOnSound = DOG::AssetManager::Get().LoadAudio("Assets/Audio/Flashlight/Flashlight_On.wav");
		m_flashlightTurnOffSound = DOG::AssetManager::Get().LoadAudio("Assets/Audio/Flashlight/Flashlight_Off.wav");
	};

	SYSTEM_CLASS(DOG::SpotLightComponent);
	ON_UPDATE(DOG::SpotLightComponent);

	void OnUpdate(DOG::SpotLightComponent& slc)
	{
		auto player = slc.owningPlayer;
		if (player == DOG::NULL_ENTITY || !DOG::EntityManager::Get().Exists(slc.owningPlayer))
			return;

		auto flashlightIsTurnedOn = DOG::EntityManager::Get().GetComponent<InputController>(player).flashlight;

		if (!DOG::EntityManager::Get().HasComponent<FlashlightSoundEffectComponent>(player))
		{
			FlashlightSoundEffectComponent& flashlightSoundEffectComponent = DOG::EntityManager::Get().AddComponent<FlashlightSoundEffectComponent>(player);

			flashlightSoundEffectComponent.flashlightAudioEntity = DOG::EntityManager::Get().CreateEntity();
			DOG::EntityManager::Get().AddComponent<DOG::TransformComponent>(flashlightSoundEffectComponent.flashlightAudioEntity);
			DOG::EntityManager::Get().AddComponent<DOG::AudioComponent>(flashlightSoundEffectComponent.flashlightAudioEntity).is3D = true;
			DOG::EntityManager::Get().AddComponent<ChildComponent>(flashlightSoundEffectComponent.flashlightAudioEntity).parent = player;
		}

		FlashlightSoundEffectComponent& flashlightSoundEffectComponent = DOG::EntityManager::Get().GetComponent<FlashlightSoundEffectComponent>(player);

		if (flashlightSoundEffectComponent.flashlightIsTurnedOn != flashlightIsTurnedOn)
		{
			DOG::AudioComponent& audio = DOG::EntityManager::Get().GetComponent<DOG::AudioComponent>(flashlightSoundEffectComponent.flashlightAudioEntity);
			if (flashlightIsTurnedOn)
			{
				audio.assetID = m_flashlightTurnOnSound;
			}
			else
			{
				audio.assetID = m_flashlightTurnOffSound;
			}
			audio.shouldPlay = true;

			flashlightSoundEffectComponent.flashlightIsTurnedOn = flashlightIsTurnedOn;
		}

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
		constexpr const float xOffset = -80.0f;
		constexpr const float yOffset = 40.0f;
		const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
		const float centerYOfScreen = (float)(abs(r.bottom - r.top)) * 0.5f;
		pos.x = r.left + centerXOfScreen + xOffset;
		pos.y = r.top + centerYOfScreen + yOffset;

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Reload text", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));
			ImGui::SetWindowFontScale(1.7f);
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
	SYSTEM_CLASS(ChildComponent, DOG::TransformComponent);

	ON_UPDATE_ID(ChildComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, ChildComponent& child, DOG::TransformComponent& world);

	ON_LATE_UPDATE(ChildComponent);
	void OnLateUpdate(ChildComponent& child);
};

class SetFlashLightToBoneSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(ChildToBoneComponent, DOG::TransformComponent);

	ON_UPDATE_ID(ChildToBoneComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, ChildToBoneComponent& child, DOG::TransformComponent& world);
};

class PlaceHolderDeathUISystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer, DeathUITimerComponent, SpectatorComponent);
	ON_UPDATE_ID(DOG::ThisPlayer, DeathUITimerComponent, SpectatorComponent);

	void OnUpdate(DOG::entity player, DOG::ThisPlayer&, DeathUITimerComponent& timer, SpectatorComponent& sc);
};

class DespawnSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DespawnComponent);
	ON_UPDATE_ID(DespawnComponent);

	void OnUpdate(DOG::entity e, DespawnComponent& despawn);
};

class SpectateSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer, SpectatorComponent);
	ON_UPDATE_ID(DOG::ThisPlayer, SpectatorComponent);

	void OnUpdate(DOG::entity player, DOG::ThisPlayer&, SpectatorComponent& sc);
	DOG::entity GetQueueIndexForSpectatedPlayer(DOG::entity player, const std::vector<DOG::entity>& players);
	void ChangeSuitDrawLogic(DOG::entity playerToDraw, DOG::entity playerToNotDraw);
};

class ReviveSystem : public DOG::ISystem
{
#define MAXIMUM_DISTANCE_DELTA 1.3f
#define MINIMUM_DOT_DELTA 0.85f
public:
	SYSTEM_CLASS(InputController, PlayerAliveComponent, DOG::TransformComponent);
	ON_UPDATE_ID(InputController, PlayerAliveComponent, DOG::TransformComponent);

	void OnUpdate(DOG::entity player, InputController&, PlayerAliveComponent&, DOG::TransformComponent&);
	ImVec4 DeterminePlayerColor(const char* playerName);
	void RevivePlayer(DOG::entity player);
	void ChangeSuitDrawLogic(DOG::entity playerToDraw, DOG::entity playerToNotDraw);
	void DrawProgressBar(const float progress);
	void UpdateSpectators();
};

class UpdateSpectatorQueueSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::ThisPlayer, SpectatorComponent);
	ON_EARLY_UPDATE(DOG::ThisPlayer, SpectatorComponent);

	void OnEarlyUpdate(DOG::ThisPlayer&, SpectatorComponent& sc);
};

class TimedDestructionSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(LifetimeComponent);
	ON_UPDATE_ID(LifetimeComponent);

	void OnUpdate(DOG::entity e, LifetimeComponent& lifetimeComp)
	{
		auto& [lifetime, age] = lifetimeComp;

		if (age >= lifetime && !DOG::EntityManager::Get().HasComponent<DOG::DeferredDeletionComponent>(e))
		{
			DOG::EntityManager::Get().DeferredEntityDestruction(e);
		}
		age += DOG::Time::DeltaTime<DOG::TimeType::Seconds, f32>();
	}
};

class PlayerLaserShootSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(LaserBarrelComponent, InputController);

	ON_UPDATE_ID(LaserBarrelComponent, InputController);
	void OnUpdate(DOG::entity e, LaserBarrelComponent& barrel, InputController& input);
};

class LaserShootSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(LaserBarrelComponent);

	ON_UPDATE_ID(LaserBarrelComponent);
	void OnUpdate(DOG::entity e, LaserBarrelComponent& barrel);
	ON_LATE_UPDATE_ID(LaserBarrelComponent);
	void OnLateUpdate(DOG::entity e, LaserBarrelComponent&);
};


class LaserBeamSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(LaserBeamComponent, LaserBeamVFXComponent);
	ON_UPDATE_ID(LaserBeamComponent, LaserBeamVFXComponent);

	void OnUpdate(DOG::entity e, LaserBeamComponent& laserBeam, LaserBeamVFXComponent& laserBeamVfx);

	LaserBeamSystem();
private:
	u32 m_audioAssetID{ 0 };
};


class LaserBeamVFXSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(LaserBeamVFXComponent);
	ON_UPDATE(LaserBeamVFXComponent);

	void OnUpdate(LaserBeamVFXComponent& laserBeam);
};

class LaserBulletCollisionSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(LaserBulletComponent, DOG::HasEnteredCollisionComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	ON_UPDATE_ID(LaserBulletComponent, DOG::HasEnteredCollisionComponent, DOG::RigidbodyComponent, DOG::TransformComponent);

	void OnUpdate(DOG::entity e, LaserBulletComponent& laserBullet, DOG::HasEnteredCollisionComponent&, DOG::RigidbodyComponent& rigidBody, DOG::TransformComponent& transform);
};

class DeferredSetIgnoreCollisionCheckSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DeferredSetIgnoreCollisionCheckComponent);
	ON_UPDATE_ID(DeferredSetIgnoreCollisionCheckComponent);

	void OnUpdate(DOG::entity e, DeferredSetIgnoreCollisionCheckComponent& arguments);
};

class GlowStickSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(GlowStickComponent, DOG::RigidbodyComponent);
	ON_UPDATE_ID(GlowStickComponent, DOG::RigidbodyComponent);

	void OnUpdate(DOG::entity e, GlowStickComponent& glowStick, DOG::RigidbodyComponent& rigidBody);
};


class PlayerUseEquipmentSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(InputController, PlayerAliveComponent);
	ON_UPDATE_ID(InputController, PlayerAliveComponent);

	void OnUpdate(DOG::entity e, InputController& controller, PlayerAliveComponent&);
};

class WeaponPointLightSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(WeaponLightComponent, DOG::PointLightComponent, ChildComponent);
	ON_UPDATE(WeaponLightComponent, DOG::PointLightComponent, ChildComponent);

	void OnUpdate(WeaponLightComponent&, DOG::PointLightComponent& pointLight, ChildComponent&);
};

class RemoveBulletComponentSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(BulletComponent, DOG::HasEnteredCollisionComponent);
	ON_LATE_UPDATE_ID(BulletComponent, DOG::HasEnteredCollisionComponent);
	void OnLateUpdate(DOG::entity e, BulletComponent&, DOG::HasEnteredCollisionComponent&);
};

