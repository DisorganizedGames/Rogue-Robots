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
	// The debug camera is local to the client
	if (!m_debugCamera)
		CreateDebugCamera();

	if (input.toggleMoveView)
	{
		input.toggleMoveView = false;
		player.moveView = !player.moveView;
	}

	// Create a new camera entity for this player
	if (!player.cameraEntity)
	{
		player.cameraEntity = EntityManager::Get().CreateEntity();
		EntityManager::Get().AddComponent<TransformComponent>(player.cameraEntity).SetScale(Vector3(1, 1, 1));
		EntityManager::Get().AddComponent<CameraComponent>(player.cameraEntity);
	}

	CameraComponent& camera = EntityManager::Get().GetComponent<CameraComponent>(player.cameraEntity);
	TransformComponent& cameraTransform = EntityManager::Get().GetComponent<TransformComponent>(player.cameraEntity);
	
	if (input.toggleDebug)
	{
		input.toggleDebug = false;
		m_useDebug = !m_useDebug;
		EntityManager::Get().GetComponent<TransformComponent>(m_debugCamera).worldMatrix = cameraTransform;
	}
	auto& debugCamera = EntityManager::Get().GetComponent<CameraComponent>(m_debugCamera);
	debugCamera.isMainCamera = m_useDebug;
	
	// Rotate player
	Vector3 forward = cameraTransform.GetForward();
	Vector3 right(1, 0, 0);
	if (player.moveView)
	{
		forward = GetNewForward(player);
	}
	right = s_globUp.Cross(forward);

	// Move player
	auto moveTowards = GetMoveTowards(input, forward, right);
	
	if (m_useDebug)
	{
		camera.isMainCamera = false;
		MoveDebugCamera(moveTowards, forward, right, 10.f, input);
		return;
	}

	rigidbody.linearVelocity = Vector3(
		moveTowards.x * playerStats.speed,
		rigidbody.linearVelocity.y,
		moveTowards.z * playerStats.speed
	);
	
	// Set the main camera to be ThisPlayer's camera
	if (EntityManager::Get().HasComponent<ThisPlayer>(e))
	{
		camera.isMainCamera = true;
	}

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
	transform.worldMatrix = XMMatrixLookToLH(transform.GetPosition(), camForward, s_globUp);
	transform.worldMatrix = transform.worldMatrix.Invert();
}

void PlayerMovementSystem::CreateDebugCamera() noexcept
{
	auto& entityManager = EntityManager::Get();
	m_debugCamera = entityManager.CreateEntity();
	entityManager.AddComponent<TransformComponent>(m_debugCamera);
	entityManager.AddComponent<CameraComponent>(m_debugCamera);
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

void PlayerMovementSystem::MoveDebugCamera(Vector3 moveTowards, Vector3 forward, Vector3 right, f32 speed, const InputController& input) noexcept
{
	auto& transform = EntityManager::Get().GetComponent<TransformComponent>(m_debugCamera);
	auto& camera = EntityManager::Get().GetComponent<CameraComponent>(m_debugCamera);
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

#pragma endregion