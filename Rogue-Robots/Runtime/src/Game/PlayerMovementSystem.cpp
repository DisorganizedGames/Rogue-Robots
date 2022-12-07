#include "PlayerMovementSystem.h"


using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;

#pragma region PlayerMovementSystem


PlayerMovementSystem::PlayerMovementSystem()
{
	m_jumpSound = AssetManager::Get().LoadAudio("Assets/Audio/Jump/PlayerJumpSound.wav");

	m_footstepSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Footsteps/footstep04.wav"));
	m_footstepSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Footsteps/footstep05.wav"));
	m_footstepSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Footsteps/footstep06.wav"));
	m_footstepSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Footsteps/footstep09.wav"));
}

void PlayerMovementSystem::CollectAndUpdate()
{
	EntityManager::Get().Collect<PlayerControllerComponent, PlayerStatsComponent, TransformComponent, RigidbodyComponent, InputController>().Do([this](
		entity e,
		PlayerControllerComponent& player,
		PlayerStatsComponent& playerStats,
		TransformComponent& transform,
		RigidbodyComponent& rigidbody,
		InputController& input) {
			OnUpdate(e, player, playerStats, transform, rigidbody, input);
		});
}

void PlayerMovementSystem::OnUpdate(
	Entity e,
	PlayerControllerComponent& player,
	PlayerStatsComponent& playerStats,
	TransformComponent& transform,
	RigidbodyComponent& rigidbody,
	InputController& input)
{
	auto& mgr = EntityManager::Get();

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

	CameraComponent& camera = mgr.GetComponent<CameraComponent>(player.cameraEntity);
	TransformComponent& cameraTransform = mgr.GetComponent<TransformComponent>(player.cameraEntity);

	// Set the main camera to be ThisPlayer's camera
	bool isThisPlayer = false;
	if (mgr.HasComponent<ThisPlayer>(e))
	{
		isThisPlayer = true;
		camera.isMainCamera = true;
	}
	auto IsAlive = [&mgr](Entity e) {
		return mgr.HasComponent<PlayerAliveComponent>(e) && mgr.GetComponent<PlayerAliveComponent>(e).timer < 0.f; };

	if (input.toggleDebug && IsAlive(e))
	{
		input.toggleDebug = false;
		if (player.debugCamera == DOG::NULL_ENTITY)
		{
			player.debugCamera = CreateDebugCamera(e);
			mgr.GetComponent<TransformComponent>(player.debugCamera).worldMatrix = cameraTransform;
			auto& debugCamera = mgr.GetComponent<CameraComponent>(player.debugCamera);
			debugCamera.isMainCamera = true;
		}
		else
		{
			mgr.DeferredEntityDestruction(player.debugCamera);
			player.debugCamera = DOG::NULL_ENTITY;
		}
	}
	if (IsAlive(e))
	{
		// Rotate player
		Vector3 forward = cameraTransform.GetForward();
		Vector3 right(1, 0, 0);
		if (player.moveView && isThisPlayer)
		{
			forward = GetNewForward(player);
			input.polarAngle = player.polar;
		}
		right = s_globUp.Cross(forward);

		// Move player
		auto moveTowards = GetMoveTowards(input, forward, right);

		if (player.debugCamera != DOG::NULL_ENTITY && isThisPlayer)
		{
			camera.isMainCamera = false;
			if (player.moveView)
				MoveDebugCamera(player.debugCamera, moveTowards, forward, right, 10.f, input);
			return;
		}

		MovePlayer(e, player, moveTowards, forward, rigidbody, playerStats.speed, playerStats.jumpSpeed, input);
		ApplyAnimations(e, input);

		f32 aspectRatio = (f32)Window::GetWidth() / Window::GetHeight();
		camera.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 1600.f, 0.1f);

		// Place camera 0.4 units above the player transform
		auto pos = transform.GetPosition() + Vector3(0, 0.7f, 0);
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
	camera.projMatrix = XMMatrixPerspectiveFovLH(80.f * XM_PI / 180.f, aspectRatio, 1600.f, 0.1f);

	auto pos = transform.GetPosition();
	camera.viewMatrix = XMMatrixLookToLH(pos, forward, forward.Cross(right));
}

void PlayerMovementSystem::MovePlayer(Entity e, PlayerControllerComponent& player, Vector3 moveTowards, Vector3 forward,
	RigidbodyComponent& rb, f32 speed, f32 jumpSpeed, InputController& input)
{
	auto forwardDisparity = moveTowards.Dot(forward);
	speed = forwardDisparity < -0.01f ? speed / 2.f : speed;

	rb.linearVelocity = Vector3(
		moveTowards.x * speed,
		rb.linearVelocity.y,
		moveTowards.z * speed
	);

	TransformComponent& playerTransform = EntityManager::Get().GetComponent<TransformComponent>(e);
	CapsuleColliderComponent& capsuleCollider = EntityManager::Get().GetComponent<CapsuleColliderComponent>(e);
	auto& comp = EntityManager::Get().GetComponent<AudioComponent>(e);

	Vector3 velocityDirection = rb.linearVelocity;
	velocityDirection.y = 0.0f;
	Vector3 oldVelocity = velocityDirection;
	velocityDirection.Normalize();

	//This check is here because otherwise bullet will crash in debug
	if (velocityDirection != Vector3::Zero)
	{
		auto rayHit = PhysicsEngine::RayCast(playerTransform.GetPosition(), playerTransform.GetPosition() + velocityDirection * capsuleCollider.capsuleRadius * 2.5f);
		if (rayHit != std::nullopt)
		{
			auto rayHitInfo = *rayHit;

			Vector3 beforeChange = oldVelocity;
			oldVelocity += rayHitInfo.hitNormal * oldVelocity.Length();

			//This section is for checking if the velocity has flipped signed value, and if it has then we want to set the velocity to zero
			const i32 getSign = 0x80000000;
			//Float hacks
			i32 beforeChangeX = std::bit_cast<i32>(beforeChange.x);
			i32 beforeChangeZ = std::bit_cast<i32>(beforeChange.z);
			i32 oldVX = std::bit_cast<i32>(oldVelocity.x);
			i32 oldVZ = std::bit_cast<i32>(oldVelocity.z);

			//Get the signed value from the floats
			bool bX = (beforeChangeX & getSign);
			bool oX = (oldVX & getSign);
			bool bZ = (beforeChangeZ & getSign);
			bool oZ = (oldVZ & getSign);

			//Check if they differ, and if they do then we set to zero
			if (bX ^ oX)
				oldVelocity.x = 0.0f;
			if (bZ ^ oZ)
				oldVelocity.z = 0.0f;

			rb.linearVelocity.x = oldVelocity.x;
			rb.linearVelocity.z = oldVelocity.z;
		}
	}

	if (input.up && !player.jumping)
	{
		const f32 heightChange = 1.2f;
		const f32 normalDirectionDifference = 0.5f;

		//Shoot a ray cast down, will miss sometimes
		auto rayHit = PhysicsEngine::RayCast(playerTransform.GetPosition(), playerTransform.GetPosition() - playerTransform.GetUp() * capsuleCollider.capsuleHeight / heightChange);

		if (rayHit != std::nullopt)
		{
			auto rayHitInfo = *rayHit;
			if (rayHitInfo.hitNormal.Dot(playerTransform.GetUp()) > normalDirectionDifference)
			{
				const f32 jumpVolume = 0.24f;

				player.jumping = true;
				rb.linearVelocity.y = jumpSpeed;

				comp.volume = jumpVolume;
				comp.assetID = m_jumpSound;
				comp.is3D = true;
				comp.shouldPlay = true;
			}
		}

		AnimationComponent& ac = EntityManager::Get().GetComponent<AnimationComponent>(e);
		const auto setterIdx = ac.addedSetters;
		ac.SimpleAdd(static_cast<i8>(MixamoAnimations::JumpForward));
		auto& s = ac.animSetters[setterIdx];
		s.transitionLength = 0.2f;
		s.playbackRate = 1.1f;
	}

	if (!player.jumping && moveTowards != Vector3::Zero && !comp.playing && m_timeBeteenTimer < Time::ElapsedTime())
	{
		const f32 footstepVolume = 0.1f;

		comp.volume = footstepVolume;
		comp.assetID = m_footstepSounds[m_changeSound];
		comp.is3D = true;
		comp.shouldPlay = true;

		m_timeBeteenTimer = m_timeBetween + (f32)Time::ElapsedTime();
		srand((unsigned)time(NULL));
		u32 oldChangeSound = m_changeSound;
		m_changeSound = rand() % m_footstepSounds.size();
		if (m_changeSound == oldChangeSound)
		{
			m_changeSound = ++oldChangeSound % m_footstepSounds.size();
		}
	}
}

void PlayerMovementSystem::ApplyAnimations(Entity e, const InputController& input)
{
	AnimationComponent& ac = EntityManager::Get().GetComponent<AnimationComponent>(e);
	auto addedAnims = 0;
	auto& setter = ac.animSetters[ac.addedSetters];
	setter.group = ac.FULL_BODY;

	auto forwardBack = input.forward - input.backwards;
	auto leftRight = input.right - input.left;
	if (forwardBack)
	{
		const auto animation = input.forward ? MixamoAnimations::Run : MixamoAnimations::WalkBackwards;
		const auto weight = 0.5f;

		setter.animationIDs[addedAnims] = static_cast<i8>(animation);
		setter.targetWeights[addedAnims++] = weight;
	}
	if (leftRight)
	{
		const auto animation = input.left ? MixamoAnimations::StrafeLeftFast : MixamoAnimations::StrafeRightFast;

		// Backwards + strafe_right makes leg clip through each other if equal weights
		auto weight = (forwardBack && input.backwards && input.right) ? 0.7f : 0.5f;

		setter.animationIDs[addedAnims] = static_cast<i8>(animation);
		setter.targetWeights[addedAnims++] = weight;
	}

	// if no movement apply idle animation
	if (!addedAnims)
	{
		ac.SimpleAdd(static_cast<i8>(MixamoAnimations::Idle), AnimationFlag::Looping);
	}
	else
	{
		setter.flag = AnimationFlag::Looping;
		// misc variables
		setter.playbackRate = 1.25f;
		setter.transitionLength = 0.1f;
		++ac.addedSetters;
	}

	// Simple aiming animation
	{
		auto aimAnimation = input.polarAngle < DirectX::XM_PIDIV2 ? MixamoAnimations::IdleHigh : MixamoAnimations::IdleLow;

		auto& s = ac.animSetters[ac.addedSetters++];
		s.group = ac.UPPER_BODY;
		s.flag = AnimationFlag::Looping;
		s.animationIDs[0] = static_cast<i8>(MixamoAnimations::Idle);
		s.animationIDs[1] = static_cast<i8>(aimAnimation);

		auto weight = abs(input.polarAngle - DirectX::XM_PIDIV2) / DirectX::XM_PIDIV2;
		s.targetWeights[0] = 1.f - weight;
		s.targetWeights[1] = weight;
		s.transitionLength = 0.f;
		s.playbackRate = 1.f;
	}
}

#pragma endregion