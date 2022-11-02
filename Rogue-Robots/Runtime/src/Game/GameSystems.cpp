#include "GameSystems.h"

using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;

#pragma region PlayerMovementSystem

void PlayerMovementSystem::OnEarlyUpdate(
	Entity e,
	PlayerControllerComponent& player,
	PlayerStatsComponent& playerStats,
	TransformComponent& transform,
	RigidbodyComponent& rigidbody,
	InputController& input)
{
	if (input.toggleMoveView)
	{
		input.toggleMoveView = false;
		player.moveView = !player.moveView;
	}

	// Create a new camera entity for this player
	if (player.cameraEntity == DOG::NULL_ENTITY)
	{
		player.cameraEntity = CreatePlayerCameraEntity(e);
	}

	CameraComponent& camera = EntityManager::Get().GetComponent<CameraComponent>(player.cameraEntity);
	TransformComponent& cameraTransform = EntityManager::Get().GetComponent<TransformComponent>(player.cameraEntity);

	// Set the main camera to be ThisPlayer's camera
	bool isThisPlayer = false;
	if (EntityManager::Get().HasComponent<ThisPlayer>(e))
	{
		isThisPlayer = true;
		camera.isMainCamera = true;
	}

	if (input.toggleDebug && EntityManager::Get().HasComponent<PlayerAliveComponent>(e))
	{
		input.toggleDebug = false;
		if (player.debugCamera == DOG::NULL_ENTITY)
		{
			player.debugCamera = CreateDebugCamera(e);
			EntityManager::Get().GetComponent<TransformComponent>(player.debugCamera).worldMatrix = cameraTransform;
			auto& debugCamera = EntityManager::Get().GetComponent<CameraComponent>(player.debugCamera);
			debugCamera.isMainCamera = true;
		}
		else
		{
			EntityManager::Get().DeferredEntityDestruction(player.debugCamera);
			player.debugCamera = DOG::NULL_ENTITY;
		}
	}

	// Rotate player
	Vector3 forward = cameraTransform.GetForward();
	Vector3 right(1, 0, 0);
	if (player.moveView && isThisPlayer)
	{
		forward = GetNewForward(player);
	}
	right = s_globUp.Cross(forward);

	// Move player
	auto moveTowards = GetMoveTowards(input, forward, right);

	if (player.debugCamera != DOG::NULL_ENTITY)
	{
		camera.isMainCamera = false;
		MoveDebugCamera(player.debugCamera, moveTowards, forward, right, 10.f, input);
		return;
	}

	MovePlayer(e, player, moveTowards, forward, rigidbody, playerStats.speed, input);

	f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
	camera.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 800.f, 0.1f);

	// Place camera 0.4 units above the player transform
	auto pos = transform.GetPosition() + Vector3(0, 0.4f, 0);
	camera.viewMatrix = XMMatrixLookToLH(pos, forward, forward.Cross(right));
	cameraTransform.worldMatrix = camera.viewMatrix.Invert();

	// Update the player transform rotation around the Y-axis to match the camera's
	auto camForward = cameraTransform.GetForward();
	camForward.y = 0;
	camForward.Normalize();

	auto prevScale = transform.GetScale();
	transform.worldMatrix = XMMatrixLookToLH(transform.GetPosition(), camForward, s_globUp);
	transform.worldMatrix = transform.worldMatrix.Invert();
	transform.SetScale(prevScale);
}

PlayerMovementSystem::Entity PlayerMovementSystem::CreateDebugCamera(Entity e) noexcept
{
	Entity debugCamera = EntityManager::Get().CreateEntity();
	if (EntityManager::Get().HasComponent<SceneComponent>(e))
	{
		EntityManager::Get().AddComponent<SceneComponent>(debugCamera,
			EntityManager::Get().GetComponent<SceneComponent>(e).scene);
	}

	EntityManager::Get().AddComponent<TransformComponent>(debugCamera);
	EntityManager::Get().AddComponent<CameraComponent>(debugCamera);

	return debugCamera;
}

PlayerMovementSystem::Entity PlayerMovementSystem::CreatePlayerCameraEntity(Entity player) noexcept
{
	Entity playerCamera = EntityManager::Get().CreateEntity();
	if (EntityManager::Get().HasComponent<SceneComponent>(player))
	{
		EntityManager::Get().AddComponent<SceneComponent>(playerCamera,
			EntityManager::Get().GetComponent<SceneComponent>(player).scene);
	}

	EntityManager::Get().AddComponent<TransformComponent>(playerCamera).SetScale(Vector3(1, 1, 1));
	EntityManager::Get().AddComponent<CameraComponent>(playerCamera);

	return playerCamera;
}

Vector3 PlayerMovementSystem::GetNewForward(PlayerControllerComponent& player) const noexcept
{
	auto [mouseX, mouseY] = DOG::Mouse::GetDeltaCoordinates();

	player.azimuthal -= mouseX * player.mouseSensitivity * XM_2PI;
	player.polar += mouseY * player.mouseSensitivity * XM_2PI;

	player.polar = std::clamp(player.polar, 0.0001f, XM_PI - 0.0001f);
	Vector3 forward = XMVectorSet(
		std::cos(player.azimuthal) * std::sin(player.polar),
		std::cos(player.polar),
		std::sin(player.azimuthal) * std::sin(player.polar),
		0
	);

	return forward;
}

Vector3 PlayerMovementSystem::GetMoveTowards(const InputController& input, Vector3 forward, Vector3 right) const noexcept
{
	Vector3 xzForward = forward;
	xzForward.y = 0;
	xzForward.Normalize();

	Vector3 moveTowards = Vector3(0, 0, 0);

	auto forwardBack = input.forward - input.backwards;
	auto leftRight = input.right - input.left;

	moveTowards = (f32)forwardBack * xzForward + (f32)leftRight * right;

	moveTowards.Normalize();

	return moveTowards;
}

void PlayerMovementSystem::MoveDebugCamera(Entity e, Vector3 moveTowards, Vector3 forward, Vector3 right, f32 speed, const InputController& input) noexcept
{
	auto& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	auto& camera = EntityManager::Get().GetComponent<CameraComponent>(e);
	camera.isMainCamera = true;

	transform.SetPosition((transform.GetPosition() += moveTowards * speed * (f32)Time::DeltaTime()));

	if (input.up)
		transform.SetPosition(transform.GetPosition() += s_globUp * speed * (f32)Time::DeltaTime());

	if (input.down)
		transform.SetPosition(transform.GetPosition() -= s_globUp * speed * (f32)Time::DeltaTime());

	f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
	camera.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 800.f, 0.1f);

	auto pos = transform.GetPosition();
	camera.viewMatrix = XMMatrixLookToLH(pos, forward, forward.Cross(right));
}

void PlayerMovementSystem::MovePlayer(Entity, PlayerControllerComponent& player, Vector3 moveTowards, Vector3 forward,
	RigidbodyComponent& rb, f32 speed, InputController& input)
{	
	auto forwardDisparity = moveTowards.Dot(forward);
	speed = forwardDisparity < -0.01f ? speed / 2.f : speed;

	rb.linearVelocity = Vector3(
		moveTowards.x * speed,
		rb.linearVelocity.y,
		moveTowards.z * speed
	);

	if (input.up && !player.jumping)
	{
		player.jumping = true;
		rb.linearVelocity.y = 6.f;
	}
}

#pragma endregion