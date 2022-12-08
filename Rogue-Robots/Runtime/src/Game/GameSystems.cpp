#include "GameSystems.h"
#include "PlayerManager/PlayerManager.h"
#include "PlayerEquipmentFunctions.h"

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

void PlayerMovementSystem::OnEarlyUpdate(
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

	if (input.toggleDebug && mgr.HasComponent<PlayerAliveComponent>(e))
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
	if (mgr.HasComponent<PlayerAliveComponent>(e))
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
			if(player.moveView)
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
	else 
	{
		// The player might be spectating, and if so the camera should be updated based on the spectated players' camera stats.
		if (mgr.HasComponent<SpectatorComponent>(e))
		{
			entity playerBeingSpectated = mgr.GetComponent<SpectatorComponent>(e).playerBeingSpectated;
			auto& spectatedPlayerControllerComponent = mgr.GetComponent<PlayerControllerComponent>(playerBeingSpectated);
			auto& spectatedCameraComponent = mgr.GetComponent<CameraComponent>(spectatedPlayerControllerComponent.cameraEntity);
			auto& spectatedCameraTransform = mgr.GetComponent<TransformComponent>(spectatedPlayerControllerComponent.cameraEntity);

			//The player is dead and so MUST have a debug camera (should be spectator camera) assigned, so:
			entity spectatorCamera = player.spectatorCamera;
			auto& spectatorCameraComponent = mgr.GetComponent<CameraComponent>(spectatorCamera);
			auto& spectatorCameraTransform = mgr.GetComponent<TransformComponent>(spectatorCamera);

			//We now simply set them equal:
			spectatorCameraComponent.viewMatrix = spectatedCameraComponent.viewMatrix;
			spectatorCameraComponent.projMatrix = spectatedCameraComponent.projMatrix;
			spectatorCameraTransform.worldMatrix = spectatedCameraTransform.worldMatrix;

			spectatorCameraComponent.isMainCamera = true;
			mgr.GetComponent<CameraComponent>(player.cameraEntity).isMainCamera = false;
			if (mgr.Exists(player.debugCamera))
			{
				mgr.GetComponent<CameraComponent>(player.debugCamera).isMainCamera = false;
			}
		}
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



void UpdateParentNode(entity parent)
{
	auto& em = EntityManager::Get();
	assert(em.HasComponent<TransformComponent>(parent));
	if (auto parentAsChild = em.TryGetComponent<ChildComponent>(parent); parentAsChild && !parentAsChild->get().nodeHasBeenUpdated)
	{
		entity grandParent = parentAsChild->get().parent;
		if (em.Exists(grandParent)) // Grand parent might have been removed. In this case we ignore it.
		{
			UpdateParentNode(grandParent);
			auto& grandParentWorld = em.GetComponent<TransformComponent>(grandParent);
			auto& parentWorld = em.GetComponent<TransformComponent>(parent);
			parentWorld.worldMatrix = parentAsChild->get().localTransform * grandParentWorld.worldMatrix;
			parentAsChild->get().nodeHasBeenUpdated = true;
		}
	}
}

void ScuffedSceneGraphSystem::OnUpdate(entity e, ChildComponent& child, TransformComponent& world)
{
	auto& em = EntityManager::Get();
	if (em.Exists(child.parent) && !em.HasComponent<DOG::DeferredDeletionComponent>(child.parent))
	{
		if (!child.nodeHasBeenUpdated)
		{
			UpdateParentNode(child.parent);
			auto& parentWorld = em.GetComponent<TransformComponent>(child.parent);
			world.worldMatrix = child.localTransform * parentWorld.worldMatrix;
			child.nodeHasBeenUpdated = true;
		}
	}
	else
	{
		em.DeferredEntityDestruction(e);
	}
}


void ScuffedSceneGraphSystem::OnLateUpdate(ChildComponent& child)
{
	child.nodeHasBeenUpdated = false;
}
void DespawnSystem::OnUpdate(DOG::entity e, DespawnComponent& despawn)
{
	if (despawn.despawnTimer < Time::ElapsedTime())
	{
		EntityManager::Get().DeferredEntityDestruction(e);
		EntityManager::Get().RemoveComponent<DespawnComponent>(e);
	}
}

void PlaceHolderDeathUISystem::OnUpdate(DOG::entity player, DOG::ThisPlayer&, DeathUITimerComponent& timer, SpectatorComponent& sc)
{
	//TODO: Also query for "being revived component" later, since that UI will otherwise probably overlap and mess up this one.
	auto& mgr = DOG::EntityManager::Get();

	if (mgr.HasComponent<PlayerAliveComponent>(player))
		return;

	if (timer.timeLeft <= 0.0f)
	{
		mgr.RemoveComponent<DeathUITimerComponent>(player);
		return;
	}

	constexpr const float timerEnd = 0.0f;
	constexpr const float alphaStart = 150.0f;
	constexpr const float alphaEnd = 0.0f;

	float alpha = DOG::Remap(timerEnd, timer.duration, alphaEnd, alphaStart, timer.timeLeft);
	timer.timeLeft -= (float)DOG::Time::DeltaTime();

	constexpr ImVec2 size {700, 450};

	auto r = DOG::Window::GetWindowRect();
	const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
	const float centerYOfScreen = (float)(abs(r.bottom - r.top)) * 0.5f;

	ImVec2 pos;
	pos.x = r.left + centerXOfScreen;
	pos.y = r.top + centerYOfScreen;
	const float xOffset = size.x / 2;
	const float yOffset = size.y / 4;

	pos.x -= xOffset;
	pos.y -= yOffset;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Death text", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		ImGui::PushFont(DOG::Window::GetFont());
		ImGui::SetWindowFontScale(4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, alpha));
		auto windowWidth = ImGui::GetWindowSize().x;
		auto textWidth = ImGui::CalcTextSize("You Died").x;

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("You Died");

		std::string specText = "Spectating " + std::string(sc.playerName);
		textWidth = ImGui::CalcTextSize(specText.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text(specText.c_str());
		ImGui::PopFont();
		ImGui::PopStyleColor(1);
	}
	ImGui::End();
	ImGui::PopStyleColor();
}

void SpectateSystem::OnUpdate(DOG::entity player, DOG::ThisPlayer&, SpectatorComponent& sc)
{
	ASSERT(!DOG::EntityManager::Get().HasComponent<PlayerAliveComponent>(player), "System should only run for dead players.");

	//Before anything we must verify that the spectator "queue" is updated according to the game state:
	for (auto i = std::ssize(sc.playerSpectatorQueue) - 1; i >= 0; --i)
	{
		if (!DOG::EntityManager::Get().HasComponent<PlayerAliveComponent>(sc.playerSpectatorQueue[i]))
		{
			if (sc.playerBeingSpectated == sc.playerSpectatorQueue[i])
			{
				//Not eligible for spectating anymore, since that player has died:
				const u32 index = GetQueueIndexForSpectatedPlayer(sc.playerBeingSpectated, sc.playerSpectatorQueue);
				const u32 nextIndex = (index + 1) % sc.playerSpectatorQueue.size();
				bool isSameEntity = index == nextIndex;
				if (!isSameEntity)
				{
					ChangeSuitDrawLogic(sc.playerBeingSpectated, sc.playerSpectatorQueue[nextIndex]);
				}
				sc.playerBeingSpectated = sc.playerSpectatorQueue[nextIndex];
				sc.playerName = DOG::EntityManager::Get().GetComponent<DOG::NetworkPlayerComponent>(sc.playerSpectatorQueue[nextIndex]).playerName;
			}
			std::swap(sc.playerSpectatorQueue[i], sc.playerSpectatorQueue[sc.playerSpectatorQueue.size() - 1]);
			sc.playerSpectatorQueue.pop_back();
		}
	}
	if (sc.playerSpectatorQueue.empty())
		return;

	constexpr ImVec2 size{ 350, 120 };

	auto r = DOG::Window::GetWindowRect();
	const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
	const float centerYOfScreen = (float)(abs(r.bottom - r.top)) * 0.5f;

	ImVec2 pos;
	pos.x = r.left + centerXOfScreen - size.x / 2.0f;
	pos.y = r.top + centerYOfScreen - (centerYOfScreen / 2.5f);

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Spectate text", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		ImGui::PushFont(DOG::Window::GetFont());
		ImGui::SetWindowFontScale(2.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
		auto textWidth = ImGui::CalcTextSize("You Are Dead").x;
		auto textHeight = ImGui::CalcTextSize("You Are Dead").y;
		const float windowWidth = ImGui::GetWindowSize().x;
		const float windowHeight = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::SetCursorPosY((windowHeight - textHeight) * 0.5f);
		ImGui::Text("You Are Dead");
		ImGui::PopFont();
		ImGui::PopStyleColor(1);
	}
	ImGui::End();
	ImGui::PopStyleColor();

	ImVec4 playerColor;
	if (strcmp(sc.playerName, "Blue") == 0)
		playerColor = ImVec4(0, 0, 255, 200);
	else if (strcmp(sc.playerName, "Red") == 0)
		playerColor = ImVec4(255, 0, 0, 200);
	else if (strcmp(sc.playerName, "Green") == 0)
		playerColor = ImVec4(0, 255, 0, 200);
	else
		playerColor = ImVec4(255, 255, 0, 200);

	pos.x = r.left + centerXOfScreen - size.x / 2.0f;
	pos.y = r.top + centerYOfScreen + (centerYOfScreen / 10.0f);

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Spectate text 2", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		ImGui::PushFont(DOG::Window::GetFont());
		ImGui::SetWindowFontScale(2.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
		//const char* text = std::string("Spectating " + std::string(sc.playerName)).c_str();
		std::string text = "Spectating ";
		text += std::string(sc.playerName);

		auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
		auto textHeight = ImGui::CalcTextSize(text.c_str()).y;
		const float windowWidth = ImGui::GetWindowSize().x;
		const float windowHeight = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::SetCursorPosY((windowHeight - textHeight) * 0.0f);
		ImGui::Text("Spectating ");
		ImGui::PopStyleColor(1);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
		ImGui::Text(sc.playerName);
		ImGui::PopStyleColor(1);

		text = "[E] Toggle Player";
		textWidth = ImGui::CalcTextSize(text.c_str()).x;
		textHeight = ImGui::CalcTextSize(text.c_str()).y;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));
		ImGui::Text("[E] ");
		ImGui::PopStyleColor(1);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
		ImGui::Text("Toggle Player");
		ImGui::PopStyleColor(1);
		ImGui::PopFont();
	}
	ImGui::End();
	ImGui::PopStyleColor();

	//Let's check for player toggle query: 
	if (!DOG::EntityManager::Get().HasComponent<InteractionQueryComponent>(player))
		return;

	const u32 index = GetQueueIndexForSpectatedPlayer(sc.playerBeingSpectated, sc.playerSpectatorQueue);
	const u32 nextIndex = (index + 1) % sc.playerSpectatorQueue.size();
	const bool isSamePlayer = (sc.playerBeingSpectated == sc.playerSpectatorQueue[nextIndex]);
	if (!isSamePlayer)
	{
		DOG::EntityManager::Get().RemoveComponent<AudioListenerComponent>(sc.playerBeingSpectated);

		ChangeSuitDrawLogic(sc.playerBeingSpectated, sc.playerSpectatorQueue[nextIndex]);
		sc.playerName = DOG::EntityManager::Get().GetComponent<DOG::NetworkPlayerComponent>(sc.playerSpectatorQueue[nextIndex]).playerName;
		sc.playerBeingSpectated = sc.playerSpectatorQueue[nextIndex];

		DOG::EntityManager::Get().AddComponent<AudioListenerComponent>(sc.playerBeingSpectated);
	}
}

u32 SpectateSystem::GetQueueIndexForSpectatedPlayer(DOG::entity player, const std::vector<DOG::entity>& players)
{
	auto it = std::find(players.begin(), players.end(), player);
	ASSERT(it != players.end(), "Couldn't find player in spectator queue.");
	return (u32)(it - players.begin());
}

void SpectateSystem::ChangeSuitDrawLogic(DOG::entity playerToDraw, DOG::entity playerToNotDraw)
{
	#if defined _DEBUG
	bool removedSuitFromRendering = false;
	bool addedSuitToRendering = false;
	#endif

	DOG::EntityManager::Get().Collect<ChildComponent>().Do([&](DOG::entity playerModel, ChildComponent& cc)
		{
			if (cc.parent == playerToDraw)
			{
				//This means that playerModel is the mesh model (suit), and it should be rendered again:
				DOG::EntityManager::Get().RemoveComponent<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				addedSuitToRendering = true;
				#endif
			}
			else if (cc.parent == playerToNotDraw)
			{
				//This means that playerModel is the spectated players' armor/suit, and it should not be eligible for rendering anymore:
				DOG::EntityManager::Get().AddComponent<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				removedSuitFromRendering = true;
				#endif
			}
		});
	#if defined _DEBUG
	ASSERT(removedSuitFromRendering && addedSuitToRendering, "Suits were not updated correctly for rendering.");
	#endif
}

void ReviveSystem::OnUpdate(DOG::entity player, InputController& inputC, PlayerAliveComponent&, DOG::TransformComponent& tc)
{
	DOG::EntityManager& mgr = DOG::EntityManager::Get();

	//If the player that wants to perform a revive does not have a reviver active item, we ofc return:
	auto optionalItem = mgr.TryGetComponent<ActiveItemComponent>(player);
	if (!optionalItem)
		return;
	if ((*optionalItem).get().type != ActiveItemComponent::Type::Reviver)
		return;

	DOG::entity closestDeadPlayer{ NULL_ENTITY };
	float distanceToClosestDeadPlayer{ FLT_MAX };

	mgr.Collect<NetworkPlayerComponent, DOG::TransformComponent>().Do([&](DOG::entity player, NetworkPlayerComponent&, DOG::TransformComponent& otc)
		{
			if (mgr.HasComponent<PlayerAliveComponent>(player))
				return;

			//This player does not live, since it passed the guard clause:
			float distanceToDeadPlayer = Vector3::Distance(tc.GetPosition(), otc.GetPosition());
			if (distanceToDeadPlayer < distanceToClosestDeadPlayer)
			{
				distanceToClosestDeadPlayer = distanceToDeadPlayer;
				closestDeadPlayer = player;
			}
		});

	//If all players live, this system should early out:
	const bool allPlayersLive = (closestDeadPlayer == NULL_ENTITY);
	if (allPlayersLive)
	{
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(player);
		return;
	}

	//Any player not part of the reviving/being revived "cooperation" should return:
	const bool isNonParticipatingPlayer = (!mgr.HasComponent<ThisPlayer>(player) && !mgr.HasComponent<ThisPlayer>(closestDeadPlayer));
	if (isNonParticipatingPlayer)
		return;

	//Likewise, if we are not close enough to a dead player, then this system should not continue:
	if (distanceToClosestDeadPlayer > MAXIMUM_DISTANCE_DELTA)
	{
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(player);
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(closestDeadPlayer);
		return;
	}

	//The last test is checking whether the player is also looking at the dead player. If not, return:
	auto deadPlayerPos = mgr.GetComponent<DOG::TransformComponent>(closestDeadPlayer).GetPosition();
	auto cam = mgr.GetComponent<PlayerControllerComponent>(player).cameraEntity;
	auto& playerCamera = mgr.GetComponent<DOG::TransformComponent>(cam);
	auto playerCameraPosition = playerCamera.GetPosition();
	auto playerCameraForward = playerCamera.GetForward();
	auto directionFromDeadToLivePlayer = deadPlayerPos - playerCameraPosition;
	directionFromDeadToLivePlayer.Normalize();
	float dot = directionFromDeadToLivePlayer.Dot(playerCameraForward);
	if (dot < MINIMUM_DOT_DELTA)
	{
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(player);
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(closestDeadPlayer);
		return;
	}

	constexpr ImVec2 size{ 700, 450 };
	auto r = DOG::Window::GetWindowRect();
	const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
	const float centerYOfScreen = (float)(abs(r.bottom - r.top)) * 0.5f;

	ImVec2 pos;
	pos.x = r.left + centerXOfScreen;
	pos.y = r.top + centerYOfScreen;
	const float xOffset = size.x / 2;
	const float yOffset = size.y / 4;

	pos.x -= xOffset;
	pos.y -= yOffset;

	if (mgr.HasComponent<ThisPlayer>(player))
	{
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowSize(size);
		ImGui::SetNextWindowPos(pos);
		if (ImGui::Begin("Revive text 1", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::SetWindowFontScale(1.75f);
			auto windowWidth = ImGui::GetWindowSize().x;
			auto& npc = mgr.GetComponent<NetworkPlayerComponent>(closestDeadPlayer);

			std::string text0 = inputC.revive ? "" : "[E] ";
			std::string text1 = inputC.revive ? std::string("Reviving ") : std::string("Revive ");
			std::string text2 = std::string(npc.playerName);
			auto textWidth = ImGui::CalcTextSize((text0 + text1 + text2).c_str()).x;

			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 175));
			ImGui::Text(text0.c_str());
			ImGui::PopStyleColor(1);

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 175));
			ImGui::Text(text1.c_str());
			ImGui::PopStyleColor(1);

			ImGui::PushStyleColor(ImGuiCol_Text, DeterminePlayerColor(npc.playerName));
			ImGui::SameLine();
			ImGui::Text(text2.c_str());

			ImGui::PopStyleColor(1);
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	//Add revive sound effect
	if (!mgr.HasComponent<ReviveSoundEffectComponent>(player))
	{
		auto& reviveComponent = mgr.AddComponent<ReviveSoundEffectComponent>(player);
		reviveComponent.reviveAudioEntity = mgr.CreateEntity();
		mgr.AddComponent<SceneComponent>(reviveComponent.reviveAudioEntity, mgr.GetComponent<SceneComponent>(player).scene);

		mgr.AddComponent<DOG::AudioComponent>(reviveComponent.reviveAudioEntity).is3D = true;
		mgr.AddComponent<DOG::TransformComponent>(reviveComponent.reviveAudioEntity);
		mgr.AddComponent<ChildComponent>(reviveComponent.reviveAudioEntity).parent = player;

		m_reviveSound = AssetManager::Get().LoadAudio("Assets/Audio/Items/Revive.wav");
	}
	auto& reviveAudioComponent = mgr.GetComponent<DOG::AudioComponent>(mgr.GetComponent<ReviveSoundEffectComponent>(player).reviveAudioEntity);

	//Next up is the revival progress. Holding E adds to the progress bar.
	//Perhaps the player is not trying to revive:
	if (!inputC.revive)
	{
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(player);
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(closestDeadPlayer);
		reviveAudioComponent.shouldStop = true;
		return;
	}

	//Play if reviving
	if (!reviveAudioComponent.playing)
	{
		reviveAudioComponent.assetID = m_reviveSound;
		reviveAudioComponent.shouldPlay = true;
	}
	
	if (mgr.HasComponent<ThisPlayer>(closestDeadPlayer))
	{
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Revive text 2", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushFont(DOG::Window::GetFont());
			ImGui::SetWindowFontScale(1.75f);
			auto windowWidth = ImGui::GetWindowSize().x;
			auto textWidth = ImGui::CalcTextSize("You are being revived by ").x;
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 175));
			ImGui::Text("You are being revived by ");
			ImGui::PopStyleColor(1);
			const char* revivingPlayerName = mgr.GetComponent<NetworkPlayerComponent>(player).playerName;
			ImGui::PushStyleColor(ImGuiCol_Text, DeterminePlayerColor(revivingPlayerName));
			ImGui::SameLine();
			ImGui::Text(revivingPlayerName);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	constexpr const float timerEnd = 0.0f;
	constexpr const float progressBarStart = 0.0f;
	constexpr const float progressBarEnd = 1.0f;

	auto& reviveComponent = mgr.AddOrGetComponent<ReviveTimerComponent>(player);
	float progress = 1.0f - DOG::Remap(timerEnd, reviveComponent.duration, progressBarStart, progressBarEnd, reviveComponent.timeLeft);
	progress = std::clamp(progress, 0.0f, 1.0f);
	reviveComponent.timeLeft -= (float)DOG::Time::DeltaTime();
	reviveComponent.timeLeft = std::clamp(reviveComponent.timeLeft, 0.0f, reviveComponent.duration);

	DrawProgressBar(progress);
	const bool revivalCompleted = (progress >= 1.0f);
	if (revivalCompleted)
	{
		//Reset UI.
		auto UIInstance = DOG::UI::Get();
		//Passive items
		UIInstance->GetUI<UIBuffTracker>(buffID)->DeactivateIcon(0);
		UIInstance->GetUI<UIBuffTracker>(buffID)->DeactivateIcon(1);
		UIInstance->GetUI<UIBuffTracker>(buffID)->DeactivateIcon(2);

		//Active item
		UIInstance->GetUI<UIIcon>(iconActiveID)->Hide();
		UIInstance->GetUI<UIIcon>(iconActiveID)->ActivateBorder();
		UIInstance->GetUI<UILabel>(lActiveItemTextID)->SetDraw(true);

		//Components
		UIInstance->GetUI<UIIcon>(iconID)->Hide();
		UIInstance->GetUI<UIIcon>(iconID)->ActivateBorder();
		UIInstance->GetUI<UIIcon>(icon2ID)->Hide();
		UIInstance->GetUI<UIIcon>(icon2ID)->ActivateBorder();
		UIInstance->GetUI<UIIcon>(icon3ID)->Hide();
		UIInstance->GetUI<UIIcon>(icon3ID)->ActivateBorder();

		//Weaponicon
		UIInstance->GetUI<UIIcon>(iconGun)->Show(0);
		UIInstance->GetUI<UIIcon>(glowstickID)->Show(0);
		UIInstance->GetUI<UIIcon>(flashlightID)->Show(0);

		// Apply Revive Animation
		{
			static constexpr u8 REVIVE_ANIMATION = 0; // No dedicated animation yet, transition to idle for now
			auto& ac = mgr.GetComponent<AnimationComponent>(closestDeadPlayer);
			ac.SimpleAdd(REVIVE_ANIMATION, AnimationFlag::Looping | AnimationFlag::ResetPrio);
		}

		if (mgr.HasComponent<ThisPlayer>(closestDeadPlayer))
		{
			auto spectatedPlayer = mgr.GetComponent<SpectatorComponent>(closestDeadPlayer).playerBeingSpectated;
			ChangeSuitDrawLogic(spectatedPlayer, closestDeadPlayer);
			RevivePlayer(closestDeadPlayer);
		}
		if (mgr.HasComponent<ThisPlayer>(player))
		{
			mgr.RemoveComponent<ActiveItemComponent>(player);
			DOG::UI::Get()->GetUI<UIIcon>(iconActiveID)->Hide();
		}
	}
}

ImVec4 ReviveSystem::DeterminePlayerColor(const char* playerName)
{
	if (strcmp(playerName, "Red") == 0)
	{
		return ImVec4(255, 0, 0, 255);
	}
	else if (strcmp(playerName, "Green") == 0)
	{
		return ImVec4(0, 255, 0, 255);
	}
	else if (strcmp(playerName, "Blue") == 0)
	{
		return ImVec4(0, 0, 255, 255);
	}
	else if (strcmp(playerName, "Yellow") == 0)
	{
		return ImVec4(255, 255, 0, 255);
	}
	else
	{
		ASSERT(false, "Invalid player name");
		return ImVec4(0, 0, 0, 255);
	}
}

//For this function we simply revert all data to an "is-alive-state":
void ReviveSystem::RevivePlayer(DOG::entity player)
{
	auto& mgr = DOG::EntityManager::Get();

	auto& psc = mgr.GetComponent<PlayerStatsComponent>(player);
	psc.health = psc.maxHealth / 2.0f;

	mgr.RemoveComponent<SpectatorComponent>(player);

	auto& pcc = mgr.GetComponent<PlayerControllerComponent>(player);
	mgr.DestroyEntity(pcc.spectatorCamera);
	pcc.spectatorCamera = NULL_ENTITY;
	mgr.GetComponent<CameraComponent>(pcc.cameraEntity).isMainCamera = true;

	mgr.AddComponent<PlayerAliveComponent>(player);
	LuaMain::GetScriptManager()->AddScript(player, "Gun.lua");
	LuaMain::GetScriptManager()->AddScript(player, "PassiveItemSystem.lua");
	LuaMain::GetScriptManager()->AddScript(player, "ActiveItemSystem.lua");

	auto& bc = mgr.AddComponent<BarrelComponent>(player);
	bc.type = BarrelComponent::Type::Bullet;
	bc.maximumAmmoCapacityForType = 999'999;
	bc.ammoPerPickup = 30;
	bc.currentAmmoCount = 30;

	auto& rb = mgr.GetComponent<RigidbodyComponent>(player);
	rb.ConstrainRotation(true, true, true);
	rb.ConstrainPosition(false, false, false);
	rb.disableDeactivation = true;
	rb.getControlOfTransform = true;
	rb.setGravityForRigidbody = true;
	rb.gravityForRigidbody = Vector3(0.0f, -25.0f, 0.0f);
}

void ReviveSystem::ChangeSuitDrawLogic(DOG::entity playerToDraw, DOG::entity playerToNotDraw)
{
	#if defined _DEBUG
	bool removedSuitFromRendering = false;
	bool addedSuitToRendering = false;
	#endif

	DOG::EntityManager::Get().Collect<ChildComponent>().Do([&](DOG::entity playerModel, ChildComponent& cc)
		{
			if (cc.parent == playerToDraw)
			{
				//This means that playerModel is the mesh model (suit), and it should be rendered again:
				DOG::EntityManager::Get().RemoveComponent<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				addedSuitToRendering = true;
				#endif
			}
			else if (cc.parent == playerToNotDraw)
			{
				//This means that playerModel is the spectated players' armor/suit, and it should not be eligible for rendering anymore:
				DOG::EntityManager::Get().AddComponent<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				removedSuitFromRendering = true;
				#endif
			}
		});
		#if defined _DEBUG
		ASSERT(removedSuitFromRendering && addedSuitToRendering, "Suits were not updated correctly for rendering.");
		#endif
}

void ReviveSystem::DrawProgressBar(const float progress)
{
	constexpr ImVec2 size{ 700, 450 };
	auto r = DOG::Window::GetWindowRect();
	const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
	const float centerYOfScreen = (float)(abs(r.bottom - r.top)) * 0.5f;

	ImVec2 pos{ r.left + centerXOfScreen, r.top + centerYOfScreen };
	constexpr const float xOffset = -150.0f;
	constexpr const float yOffset = -50.0f;
	const float finalWindowPosX = pos.x + xOffset;
	const float finalWindowPosY = pos.y + yOffset;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2{ 300, 200 });
	ImGui::SetNextWindowPos(ImVec2{ finalWindowPosX, finalWindowPosY });
	if (ImGui::Begin("Progress Bar 1", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		ImGui::PushItemWidth(300.0f);
		ImGui::SetWindowFontScale(1.20f);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_FrameBg, IM_COL32(255, 255, 255, 255));
		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
		ImGui::PopStyleColor(1);
		ImGui::PopItemWidth();
	}
	ImGui::End();
	ImGui::PopStyleColor();
}

void UpdateSpectatorQueueSystem::OnEarlyUpdate(DOG::ThisPlayer&, SpectatorComponent& sc)
{
	DOG::EntityManager::Get().Collect<PlayerAliveComponent>().Do([&](DOG::entity otherPlayer, PlayerAliveComponent&)
		{
			if (std::find(sc.playerSpectatorQueue.begin(), sc.playerSpectatorQueue.end(), otherPlayer) == sc.playerSpectatorQueue.end())
			{
				sc.playerSpectatorQueue.push_back(otherPlayer);
			}
		});
}

void PlayerLaserShootSystem::OnUpdate(entity e, LaserBarrelComponent& barrel, InputController& input)
{
	if (EntityManager::Get().Exists(GetGun()))
	{
		assert(EntityManager::Get().HasComponent<TransformComponent>(GetGun()));
		auto& tr = EntityManager::Get().GetComponent<TransformComponent>(GetGun());
		Vector3 dir = tr.GetUp();
		dir.Normalize();
		Vector3 laserStart = tr.GetPosition() + 0.8f * dir;
		barrel.shoot = input.shoot;
		barrel.laserToShoot.direction = dir;
		barrel.laserToShoot.startPos = laserStart;
		barrel.laserToShoot.owningPlayer = e;
		barrel.laserToShoot.color = 7 * Vector3(1.5f, 0.1f, 0.1f);
	}
}

void LaserShootSystem::OnUpdate(entity e, LaserBarrelComponent& barrel)
{
	auto& em = EntityManager::Get();
	
	f32 dt = static_cast<f32>(Time::DeltaTime());

	if (barrel.shoot && barrel.ammo > 0)
	{
		barrel.ammo -= dt;
		f32 dmg = barrel.damagePerSecond * (dt + std::min(0.0f, barrel.ammo));
		em.AddOrReplaceComponent<LaserBeamComponent>(e, barrel.laserToShoot).damage = dmg;
		if(!em.HasComponent<LaserBeamVFXComponent>(e)) em.AddComponent<LaserBeamVFXComponent>(e);
		barrel.shoot = false;
	}
	else
	{
		em.RemoveComponentIfExists<LaserBeamComponent>(e);
		if (auto vfxLaser = em.TryGetComponent<LaserBeamVFXComponent>(e))
		{
			if (em.Exists(vfxLaser->get().particleEmitter))
			{
				em.DeferredEntityDestruction(vfxLaser->get().particleEmitter);
			}
			em.RemoveComponent<LaserBeamVFXComponent>(e);
		}
	}
}

void LaserShootSystem::OnLateUpdate(DOG::entity e, LaserBarrelComponent&)
{
	auto& em = EntityManager::Get();
	if (em.HasComponent<DOG::DeferredDeletionComponent>(e))
	{
		em.RemoveComponentIfExists<LaserBeamComponent>(e);
		if (auto vfxLaser = em.TryGetComponent<LaserBeamVFXComponent>(e))
		{
			if (em.Exists(vfxLaser->get().particleEmitter))
			{
				em.DeferredEntityDestruction(vfxLaser->get().particleEmitter);
			}
			em.RemoveComponent<LaserBeamVFXComponent>(e);
		}
	}
}

void LaserBeamSystem::OnUpdate(entity e, LaserBeamComponent& laserBeam, LaserBeamVFXComponent& laserBeamVfx)
{
	auto& em = EntityManager::Get();
	Vector3 target = laserBeam.startPos + laserBeam.maxRange * laserBeam.direction;
	if (auto hit = PhysicsEngine::RayCast(laserBeam.startPos, target); hit)
	{
		target = hit->hitPosition;

		if (em.Exists(hit->entityHit) && em.HasComponent<AgentIdComponent>(hit->entityHit))
		{
			em.AddOrGetComponent<AgentHitComponent>(hit->entityHit).HitBy(e, laserBeam.owningPlayer, laserBeam.damage);
		}

		if (!em.Exists(laserBeamVfx.particleEmitter))
		{
			laserBeamVfx.particleEmitter = em.CreateEntity();

			if (auto scene = em.TryGetComponent<SceneComponent>(e))
			{
				em.AddComponent<SceneComponent>(laserBeamVfx.particleEmitter, scene->get().scene);
			}
		}

		auto& emitterTransform = em.AddOrGetComponent<TransformComponent>(laserBeamVfx.particleEmitter);

		Vector3 i = -laserBeam.direction;
		i.Normalize();
		Vector3 r = Vector3::Lerp(i, Vector3::Reflect(i, hit->hitNormal), 0.5f);

		emitterTransform.worldMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(target, target + r, Vector3::Up).Invert();
		emitterTransform.RotateL({ DirectX::XM_PIDIV2, 0, 0 });

		auto& coneEmitter = em.AddOrGetComponent<ConeSpawnComponent>(laserBeamVfx.particleEmitter);
		coneEmitter.speed = 3;



		auto& particleEmitter = em.AddOrGetComponent<ParticleEmitterComponent>(laserBeamVfx.particleEmitter);
		particleEmitter.particleSize = 0.07f;
		particleEmitter.spawnRate = 80.0f;
		particleEmitter.particleLifetime = 0.14f;
		particleEmitter.startColor = { laserBeam.color.x, laserBeam.color.y, laserBeam.color.z, 1 };
		particleEmitter.endColor = { laserBeam.color.x, 1.5f * laserBeam.color.y, laserBeam.color.z, 0 };


		if (!em.HasComponent<PointLightComponent>(laserBeamVfx.particleEmitter))
		{
			auto& p = em.AddComponent<PointLightComponent>(laserBeamVfx.particleEmitter);
			p.radius = 3;
			p.strength = 1;
			p.color = laserBeam.color;
			p.handle = LightManager::Get().AddPointLight(
				PointLightDesc
				{
					.position = target,
					.radius = p.radius,
					.color = p.color,
					.strength = p.strength
				},
				LightUpdateFrequency::PerFrame
			);
		}
		else
		{
			auto& p = em.GetComponent<PointLightComponent>(laserBeamVfx.particleEmitter);
			p.color = laserBeam.color;
			// Note that we do not only set dirty to true because of the color change.
			// The primary reason is to get the position to get updated later on.
			p.dirty = true; 
		}



		if (!em.HasComponent<AudioComponent>(laserBeamVfx.particleEmitter))
		{
			auto& audio = em.AddComponent<AudioComponent>(laserBeamVfx.particleEmitter);
			audio.assetID = m_audioAssetID;
			audio.loop = true;
			audio.shouldPlay = true;
			audio.loopStart = 0;
			audio.loopEnd = -1;
			audio.volume = 1;
		}
	}

	laserBeamVfx.startPos = laserBeam.startPos;
	laserBeamVfx.endPos = target;
	laserBeamVfx.color = laserBeam.color;
}

LaserBeamSystem::LaserBeamSystem()
{
	m_audioAssetID = AssetManager::Get().LoadAudio("Assets/Audio/GunSounds/LaserBeam.wav");
}

void LaserBeamVFXSystem::OnUpdate(LaserBeamVFXComponent& laserBeam)
{
	entity camera = GetCamera();
	if (camera == NULL_ENTITY) return;
	assert(EntityManager::Get().HasComponent<TransformComponent>(camera));
	Vector3 dirToCamera = EntityManager::Get().GetComponent<TransformComponent>(camera).GetPosition() - laserBeam.startPos;
	dirToCamera.Normalize();

	static std::random_device rdev;
	static std::mt19937 gen(rdev());
	static std::uniform_real_distribution<f32> udis(-1.0f, 1.0f);
	Vector3 jitter(udis(gen), udis(gen), udis(gen));

	f32 e = 3.0f * static_cast<f32>(sin(30.0 * Time::ElapsedTime()));
	f32 f = 7.0f + e;

	gfx::PostProcess::Get().InstantiateLaserBeam(laserBeam.startPos + 0.002f * jitter, laserBeam.endPos, dirToCamera, f * (laserBeam.color += 0.02f * jitter));
}

void LaserBulletCollisionSystem::OnUpdate(DOG::entity e, LaserBulletComponent& laserBullet, DOG::HasEnteredCollisionComponent& collision, DOG::RigidbodyComponent& rigidBody, DOG::TransformComponent& transform)
{
	auto& em = EntityManager::Get();
	em.DeferredEntityDestruction(e);

	entity reflectedParticles = em.CreateEntity();
	entity randomScatterParticles = em.CreateEntity();
	if (auto scene = em.TryGetComponent<SceneComponent>(e))
	{
		em.AddComponent<SceneComponent>(reflectedParticles, scene->get().scene);
		em.AddComponent<SceneComponent>(randomScatterParticles, scene->get().scene);
	}

	Vector3 n = collision.normal[0];
	Vector3 i = rigidBody.linearVelocity;
	i.Normalize();
	Vector3 r = Vector3::Reflect(-i, n);

	Vector3 pos = transform.GetPosition();
	
	auto& tr = em.AddComponent<TransformComponent>(reflectedParticles);
	tr.worldMatrix= DirectX::SimpleMath::Matrix::CreateLookAt(pos, pos + r, Vector3::Up).Invert();
	tr.RotateL({ DirectX::XM_PIDIV2, 0, 0 });

	auto& tr2 = em.AddComponent<TransformComponent>(randomScatterParticles) = tr;
	tr2.RotateL({ DirectX::XM_PI, 0, 0 });


	em.AddComponent<LifetimeComponent>(reflectedParticles, 0.04f);
	em.AddComponent<LifetimeComponent>(randomScatterParticles, 0.04f);


	Vector4 startColor = 2.5f * Vector4(laserBullet.color.x, 1.6f * laserBullet.color.y, laserBullet.color.z, 1);
	startColor.w = 1;
	Vector4 endColor = 0.3f * Vector4(0.3f * laserBullet.color.x, 0.8f * laserBullet.color.y, laserBullet.color.z, 1);
	endColor.w = 0.3f;


	em.AddComponent<ConeSpawnComponent>(reflectedParticles) = { .angle = DirectX::XM_PI / 5, .speed = 8.f };
	em.AddComponent<ParticleEmitterComponent>(reflectedParticles) = {
		.spawnRate = 600,
		.particleSize = 0.05f,
		.particleLifetime = 1.1f,
		.startColor = startColor,
		.endColor = endColor,
	};

	em.AddComponent<ConeSpawnComponent>(randomScatterParticles) = { .angle = DirectX::XM_PI / 1.5f, .speed = 4.f };
	em.AddComponent<ParticleEmitterComponent>(randomScatterParticles) = {
		.spawnRate = 200,
		.particleSize = 0.08f,
		.particleLifetime = 0.8f,
		.startColor = Vector4::Lerp(startColor, 2 * Vector4(1, 1, 0.7f, 0.5f), 0.5f),
		.endColor = Vector4::Lerp(startColor, 2 * Vector4(1, 1, 0.7f, 0.5f), 0.5f),
	};

}

void DeferredSetIgnoreCollisionCheckSystem::OnUpdate(DOG::entity e, DeferredSetIgnoreCollisionCheckComponent& arguments)
{
	auto& em = EntityManager::Get();
	arguments.countDown -= Time::DeltaTime<TimeType::Seconds, f32>();
	if (arguments.countDown < 0)
	{
		if (auto rbA = em.TryGetComponent<RigidbodyComponent>(e); rbA && em.Exists(arguments.other))
		{
			if (auto rbB = em.TryGetComponent<RigidbodyComponent>(arguments.other))
			{
				PhysicsEngine::SetIgnoreCollisionCheck(rbA->get().rigidbodyHandle, rbB->get().rigidbodyHandle, arguments.value);
			}
		}
		em.RemoveComponent<DeferredSetIgnoreCollisionCheckComponent>(e);
	}
}

void GlowStickSystem::OnUpdate(entity e, GlowStickComponent&, RigidbodyComponent& rigidBody)
{
	auto& em = EntityManager::Get();
	if (rigidBody.linearVelocity.LengthSquared() > 0.1f)
	{
		em.AddOrReplaceComponent<DirtyComponent>(e).SetDirty(DirtyComponent::positionChanged);
	}
}


void PlayerUseEquipmentSystem::OnUpdate(DOG::entity e, InputController& controller, PlayerAliveComponent&)
{
	auto& thrower = EntityManager::Get().AddOrGetComponent<GlowStickThrowerComponent>(e);

	if (!thrower.waitForNewKeyDown && controller.throwGlowStick)
	{
		ThrowGlowStick(e, 14);
		thrower.waitForNewKeyDown = true;
	}
	else if (thrower.waitForNewKeyDown && !controller.throwGlowStick)
	{
		thrower.waitForNewKeyDown = false;
	}
}


void SetFlashLightToBoneSystem::OnUpdate(DOG::entity e, ChildToBoneComponent& child, DOG::TransformComponent& world)
{
	auto& em = EntityManager::Get();
	if (em.Exists(child.boneParent))
	{
		auto offset = DirectX::XMMatrixTranslationFromVector({ -8.f, -7.5f, -0.1f });
		auto& boneHeadWorld = em.GetComponent<MixamoHeadJointTF>(child.boneParent);
		world.worldMatrix = Matrix(offset) * boneHeadWorld.transform;
		world.SetPosition(world.GetPosition() + Vector3(0.f, -0.5f, 0.f));
	}
	else
	{
		em.DeferredEntityDestruction(e);
	}
}

void WeaponPointLightSystem::OnUpdate(WeaponLightComponent&, DOG::PointLightComponent& pointLight, ChildComponent&)
{
	pointLight.dirty = true;
} 

void RemoveBulletComponentSystem::OnLateUpdate(DOG::entity e, BulletComponent&, DOG::HasEnteredCollisionComponent&)
{
	EntityManager::Get().RemoveComponent<BulletComponent>(e);
}

void SetPointLightDirtySystem::OnUpdate(DOG::PointLightComponent& light, SetPointLightDirtyComponent&)
{
	light.dirty = true;
}

void SetOutlineOnNearbyPickupsSystem::OnUpdate(DOG::entity e, DOG::PickupLerpAnimateComponent&, DOG::TransformComponent& transform)
{
#define PICKUP_OUTLINE_UNIT_THRESHOLD 20.f
#define PICKUP_OUTLINE_MAX_INTENSITY 1.f
#define PICKUP_OUTLINE_MIN_INTENSITY 0.1f

	MINIPROFILE
	auto& em = EntityManager::Get();

	const auto& pickupPos = transform.GetPosition();
	em.Collect<ThisPlayer, TransformComponent>().Do([&](DOG::entity, DOG::ThisPlayer&, DOG::TransformComponent& tfc)
		{
			const auto& selfPos = tfc.GetPosition();
			const auto lenToPickup = (pickupPos - selfPos).Length();

			// [0, 1]
			float intensity = lenToPickup / PICKUP_OUTLINE_UNIT_THRESHOLD;
			// [0, MAX_INTENSITY - 1]
			intensity *= (PICKUP_OUTLINE_MAX_INTENSITY - PICKUP_OUTLINE_MIN_INTENSITY);
			// [1, MAX_INTENSITY]
			intensity += PICKUP_OUTLINE_MIN_INTENSITY;

			if (lenToPickup <= PICKUP_OUTLINE_UNIT_THRESHOLD)
			{
				if (!em.HasComponent<OutlineComponent>(e))
				{
					DirectX::SimpleMath::Vector3 color{};
					if (em.HasComponent<PassiveItemComponent>(e))
						color = { 0.f, 1.f, 0.f };
					else if (em.HasComponent<ActiveItemComponent>(e))
						color = { 1.f, 1.f, 0.f };
					else if (em.HasComponent<MagazineModificationComponent>(e))
						color = { 0.35f, 0.35f, 1.f };
					else if (em.HasComponent<BarrelComponent>(e))
						color = { 0.f, 0.f, 1.f };
					else if (em.HasComponent<MiscComponent>(e))
						color = { 0.1f, 0.4f, 1.f };

					if (em.HasComponent<OutlineBabyComponent>(e))
					{
						auto& childEntity = em.GetComponent<OutlineBabyComponent>(e).child;
						if (!em.HasComponent<OutlineComponent>(childEntity))
							em.AddComponent<OutlineComponent>(childEntity).color = color * intensity;
					}

					em.AddComponent<OutlineComponent>(e).color = color * intensity;
				}
			}
			else
			{
				if (em.HasComponent<OutlineComponent>(e))
					em.RemoveComponentIfExists<OutlineComponent>(e);
			}
		});

}

#pragma endregion