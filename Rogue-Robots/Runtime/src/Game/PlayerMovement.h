#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class PlayerMovementSystem
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
	void CollectAndUpdate();

private:
	void OnUpdate(Entity, PlayerControllerComponent&, PlayerStatsComponent&, TransformComponent&, RigidbodyComponent&, InputController&);
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