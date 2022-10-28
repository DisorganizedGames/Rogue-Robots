#include "GameSystems.h"

using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;

#pragma region PlayerMovementSystem

void PlayerMovementSystem::OnUpdate(
	Entity e,
	PlayerControllerComponent& player,
	PlayerStatsComponent& playerStats,
	TransformComponent& transform, 
	RigidbodyComponent& rigidbody,
	InputController& input)
{
	if (!m_debugCamera)
		CreateDebugCamera();

	if (!player.cameraEntity)
	{
		player.cameraEntity = EntityManager::Get().CreateEntity();
		EntityManager::Get().AddComponent<TransformComponent>(player.cameraEntity).SetScale(Vector3(1, 1, 1));
		auto& camera = EntityManager::Get().AddComponent<CameraComponent>(player.cameraEntity);
		if (EntityManager::Get().HasComponent<ThisPlayer>(e))
		{
			camera.isMainCamera = true;
		}
	}
	
	Vector3 forward = transform.GetForward();
	Vector3 right(0, 0, 0);

	if (player.moveView)
	{
		forward = GetNewForward(player);
	}

	right = s_globUp.Cross(forward);

	auto moveTowards = GetMoveTowards(input, forward, right);

	rigidbody.linearVelocity = Vector3(
		moveTowards.x * playerStats.speed,
		rigidbody.linearVelocity.y,
		moveTowards.z * playerStats.speed
	);
	
	CameraComponent& camera = EntityManager::Get().GetComponent<CameraComponent>(player.cameraEntity);
	TransformComponent& cameraTransform = EntityManager::Get().GetComponent<TransformComponent>(player.cameraEntity);

	f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
	camera.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 800.f, 0.1f);
	

	auto pos = transform.GetPosition() + Vector3(0, 0.4f, 0);
	camera.viewMatrix = XMMatrixLookToLH(pos, forward, forward.Cross(right));
	cameraTransform.worldMatrix = camera.viewMatrix.Invert();

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
}

Vector3 PlayerMovementSystem::GetNewForward(PlayerControllerComponent& player) const noexcept
{
	auto [mouseX, mouseY] = DOG::Mouse::GetDeltaCoordinates();

	player.azimuthal -= mouseX * player.mouseSensitivity * XM_2PI;
	player.polar += mouseY * player.mouseSensitivity * XM_2PI;

	player.polar = std::clamp(player.polar, 0.0001f - XM_PI, XM_PI - 0.0001f);
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

#pragma endregion