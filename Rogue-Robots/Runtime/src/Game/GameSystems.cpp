#include "GameSystems.h"
#include "PlayerManager/PlayerManager.h"
#include "PlayerEquipmentFunctions.h"

using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;

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

	constexpr ImVec2 size{ 350, 100 };

	auto r = DOG::Window::GetWindowRect();
	const float centerXOfScreen = (float)(abs(r.right - r.left)) * 0.5f;
	constexpr const float yOffset = 50.0f;
	constexpr const float xOffset = -(size.x / 2);

	ImVec2 pos;
	pos.x = r.left + centerXOfScreen;
	pos.x += xOffset;
	pos.y = r.top + yOffset;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Spectate text", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		ImGui::PushFont(DOG::Window::GetFont());
		ImGui::SetWindowFontScale(2.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 75));
		auto windowWidth = ImGui::GetWindowSize().x;
		auto textWidth = ImGui::CalcTextSize("You Are Dead").x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("You Are Dead");
		ImGui::PopFont();
		ImGui::PopStyleColor(1);
	}
	ImGui::End();
	ImGui::PopStyleColor();

	ImVec4 playerColor;
	if (strcmp(sc.playerName, "Blue") == 0)
		playerColor = ImVec4(0, 0, 255, 75);
	else if (strcmp(sc.playerName, "Red") == 0)
		playerColor = ImVec4(255, 0, 0, 75);
	else if (strcmp(sc.playerName, "Green") == 0)
		playerColor = ImVec4(0, 255, 0, 75);
	else
		playerColor = ImVec4(255, 255, 0, 75);

	const float additionalXOffset = (centerXOfScreen / 2);
	const float additionalYOffset = -70.0f;

	pos.x = r.left + centerXOfScreen + additionalXOffset;
	pos.y = r.bottom - yOffset + additionalYOffset;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Spectate text 2", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		ImGui::PushFont(DOG::Window::GetFont());
		ImGui::SetWindowFontScale(2.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 75));
		ImGui::Text("Spectating ");
		ImGui::PopStyleColor(1);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
		ImGui::Text(sc.playerName);
		ImGui::PopStyleColor(1);

		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 75));
		ImGui::Text("[E] ");
		ImGui::PopStyleColor(1);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 75));
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
		ChangeSuitDrawLogic(sc.playerBeingSpectated, sc.playerSpectatorQueue[nextIndex]);
		sc.playerName = DOG::EntityManager::Get().GetComponent<DOG::NetworkPlayerComponent>(sc.playerSpectatorQueue[nextIndex]).playerName;
		sc.playerBeingSpectated = sc.playerSpectatorQueue[nextIndex];
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

	//Next up is the revival progress. Holding E adds to the progress bar.
	//Perhaps the player is not trying to revive:
	if (!inputC.revive)
	{
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(player);
		mgr.RemoveComponentIfExists<ReviveTimerComponent>(closestDeadPlayer);
		return;
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
			mgr.RemoveComponent<ActiveItemComponent>(player);
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
	f32 dt = static_cast<f32>(Time::DeltaTime());

	if (barrel.shoot && barrel.ammo > 0)
	{
		barrel.ammo -= dt;
		f32 dmg = barrel.damagePerSecond * (dt + std::min(0.0f, barrel.ammo));
		EntityManager::Get().AddOrReplaceComponent<LaserBeamComponent>(e, barrel.laserToShoot).damage = dmg;
		EntityManager::Get().AddOrReplaceComponent<LaserBeamVFXComponent>(e);
		barrel.shoot = false;
	}
	else
	{
		EntityManager::Get().RemoveComponentIfExists<LaserBeamComponent>(e);
		EntityManager::Get().RemoveComponentIfExists<LaserBeamVFXComponent>(e);
	}
}

void LaserShootSystem::OnLateUpdate(DOG::entity e, LaserBarrelComponent&)
{
	if (EntityManager::Get().HasComponent<DOG::DeferredDeletionComponent>(e))
	{
		EntityManager::Get().RemoveComponentIfExists<LaserBeamComponent>(e);
		EntityManager::Get().RemoveComponentIfExists<LaserBeamVFXComponent>(e);
	}
}

void LaserBeamSystem::OnUpdate(entity e, LaserBeamComponent& laserBeam, LaserBeamVFXComponent& laserBeamVfx)
{
	Vector3 target = laserBeam.startPos + laserBeam.maxRange * laserBeam.direction;
	if (auto hit = PhysicsEngine::RayCast(laserBeam.startPos, target); hit)
	{
		target = hit->hitPosition;

		if (EntityManager::Get().Exists(hit->entityHit) && EntityManager::Get().HasComponent<AgentIdComponent>(hit->entityHit))
		{
			EntityManager::Get().AddOrGetComponent<AgentHitComponent>(hit->entityHit).HitBy(e, laserBeam.owningPlayer, laserBeam.damage);
		}
	}

	laserBeamVfx.startPos = laserBeam.startPos;
	laserBeamVfx.endPos = target;
	laserBeamVfx.color = laserBeam.color;
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
	Vector4 endColor = 0.3f * Vector4(0.3f * laserBullet.color.x, 0.8f * laserBullet.color.y, laserBullet.color.z, 1);


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
