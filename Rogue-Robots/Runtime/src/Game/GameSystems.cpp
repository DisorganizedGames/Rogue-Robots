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
			const bool spectatedPlayerIsDead = sc.playerBeingSpectated == sc.playerSpectatorQueue[i];
			if (spectatedPlayerIsDead)
			{
				//Not eligible for spectating anymore, since that player has died:
				const u32 index = GetQueueIndexForSpectatedPlayer(sc.playerBeingSpectated, sc.playerSpectatorQueue);
				const u32 nextIndex = (index + 1) % sc.playerSpectatorQueue.size();
				bool isSameEntity = index == nextIndex;
				if (!isSameEntity) 
				{
					ChangeGunDrawLogic(sc.playerBeingSpectated, false, true);
					ChangeGunDrawLogic(sc.playerSpectatorQueue[nextIndex], true, false);
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
		DOG::EntityManager::Get().RemoveComponentIfExists<AudioListenerComponent>(sc.playerBeingSpectated);

		ChangeGunDrawLogic(sc.playerBeingSpectated, false, true);
		ChangeGunDrawLogic(sc.playerSpectatorQueue[nextIndex], true, false);
		ChangeSuitDrawLogic(sc.playerBeingSpectated, sc.playerSpectatorQueue[nextIndex]);
		sc.playerName = DOG::EntityManager::Get().GetComponent<DOG::NetworkPlayerComponent>(sc.playerSpectatorQueue[nextIndex]).playerName;
		sc.playerBeingSpectated = sc.playerSpectatorQueue[nextIndex];

		DOG::EntityManager::Get().AddOrReplaceComponent<AudioListenerComponent>(sc.playerBeingSpectated);
	}

	bool changeGunDrawLogic = false;
	DOG::EntityManager::Get().Collect<ModelComponent, ChildToBoneComponent>().Do([&](entity gunModelNotFPS, ModelComponent&, ChildToBoneComponent& childToBone)
		{
			if (!changeGunDrawLogic)
				changeGunDrawLogic = (childToBone.boneParent == sc.playerBeingSpectated && !EntityManager::Get().HasComponent<DontDraw>(gunModelNotFPS));
		});
	if (changeGunDrawLogic)
		ChangeGunDrawLogic(sc.playerBeingSpectated, true, false);
}

u32 SpectateSystem::GetQueueIndexForSpectatedPlayer(DOG::entity player, const std::vector<DOG::entity>& players)
{
	auto it = std::find(players.begin(), players.end(), player);
	ASSERT(it != players.end(), "Couldn't find player in spectator queue.");
	return (u32)(it - players.begin());
}

void SpectateSystem::ChangeGunDrawLogic(DOG::entity player, bool drawFirstPersonViewGun, bool drawModelGun)
{
	auto& em = DOG::EntityManager::Get();
	
	// Draw Logic FirstPersonView Gun
	if(em.HasComponent<ScriptComponent>(player))
	{
		auto scriptData = LuaMain::GetScriptManager()->GetScript(player, "Gun.lua");
		LuaTable tab(scriptData.scriptTable, true);
		auto ge = tab.GetTableFromTable("gunEntity");

		int gunID = ge.GetIntFromTable("entityID");
		int barrelID = tab.GetIntFromTable("barrelEntityID");
		int miscID = tab.GetIntFromTable("miscEntityID");
		int magazineID = tab.GetIntFromTable("magazineEntityID");

		if (drawFirstPersonViewGun)
		{
			em.RemoveComponentIfExists<DontDraw>(gunID);
			em.RemoveComponentIfExists<DontDraw>(barrelID);
			em.RemoveComponentIfExists<DontDraw>(miscID);
			em.RemoveComponentIfExists<DontDraw>(magazineID);
		}
		else
		{
			em.AddOrReplaceComponent<DontDraw>(gunID);
			em.AddOrReplaceComponent<DontDraw>(barrelID);
			em.AddOrReplaceComponent<DontDraw>(miscID);
			em.AddOrReplaceComponent<DontDraw>(magazineID);
		}
	}

	// Draw Logic FirstPersonView Gun
	{
		EntityManager::Get().Collect<ModelComponent, ChildToBoneComponent>().Do([&](entity modelGun, ModelComponent&, ChildToBoneComponent& bone)
			{
				if (bone.boneParent == player)
					if (drawModelGun)
						DOG::EntityManager::Get().RemoveComponentIfExists<DOG::DontDraw>(modelGun);
					else
						DOG::EntityManager::Get().AddOrReplaceComponent<DOG::DontDraw>(modelGun);
			});
	}
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
				DOG::EntityManager::Get().RemoveComponentIfExists<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				addedSuitToRendering = true;
				#endif
			}
			else if (cc.parent == playerToNotDraw)
			{
				//This means that playerModel is the spectated players' armor/suit, and it should not be eligible for rendering anymore:
				DOG::EntityManager::Get().AddOrReplaceComponent<DOG::DontDraw>(playerModel);
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

		if (mgr.HasComponent<ThisPlayer>(closestDeadPlayer) && mgr.HasComponent<SpectatorComponent>(closestDeadPlayer))
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

	/*auto& ac = mgr.GetComponent<AnimationComponent>(player);
	ac.SimpleAdd(static_cast<i8>(MixamoAnimations::StandUp), AnimationFlag::ResetPrio, ac.BASE_PRIORITY, ac.FULL_BODY, 1.5f, 0.5f);*/
	//mgr.AddComponent<PlayerAliveComponent>(player).timer = 2.f;

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
			auto& em = DOG::EntityManager::Get();
			if (cc.parent == playerToDraw)
			{
				//std::cout << "deadPlayer" << playerToDraw << std::endl;
				
				//This means that playerModel is the mesh model (suit), and it should be rendered again:
				em.RemoveComponentIfExists<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				addedSuitToRendering = true;
				#endif
				EntityManager::Get().Collect<ModelComponent, ChildToBoneComponent>().Do([&](entity modelGun, ModelComponent&, ChildToBoneComponent& bone)
					{
						//std::cout << "boneParent" << bone.boneParent << std::endl;
						if (bone.boneParent == playerToDraw)
							em.RemoveComponentIfExists<DOG::DontDraw>(modelGun);
					});

				//std::cout << "ChangeSuitDrawLogic  playerToDraw  BeforeAddOrReplaceScript"<< std::endl;
				auto scriptData = LuaMain::GetScriptManager()->GetScript(playerToDraw, "Gun.lua");
				LuaTable tab(scriptData.scriptTable, true);
				auto ge = tab.GetTableFromTable("gunEntity");
				int gunID = ge.GetIntFromTable("entityID");
				em.AddOrReplaceComponent<DontDraw>(gunID);
				int barrelID = tab.GetIntFromTable("barrelEntityID");
				em.AddOrReplaceComponent<DontDraw>(barrelID);
				int miscID = tab.GetIntFromTable("miscEntityID");
				em.AddOrReplaceComponent<DontDraw>(miscID);
				int magazineID = tab.GetIntFromTable("magazineEntityID");
				em.AddOrReplaceComponent<DontDraw>(magazineID);
			}
			else if (cc.parent == playerToNotDraw)
			{
				//std::cout << "ChangeSuitDrawLogic Before player NOT to draw" << std::endl;
				//This means that playerModel is the spectated players' armor/suit, and it should not be eligible for rendering anymore:
				DOG::EntityManager::Get().AddOrReplaceComponent<DOG::DontDraw>(playerModel);
				#if defined _DEBUG
				removedSuitFromRendering = true;
				#endif
				//std::cout << std::endl << "playerToDraw" << playerToDraw << std::endl;
				EntityManager::Get().Collect<ModelComponent, ChildToBoneComponent>().Do([&](entity modelGun, ModelComponent&, ChildToBoneComponent& bone)
					{
						//std::cout << std::endl << "boneParent" << bone.boneParent << std::endl;
						if (bone.boneParent == playerToNotDraw)
							DOG::EntityManager::Get().AddOrReplaceComponent<DOG::DontDraw>(modelGun);
					});

				//std::cout << "ChangeSuitDrawLogic  playerNotToDraw  BeforeRemoveIfExistsScript" << std::endl;
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
		if (!em.HasComponent<ModelComponent>(e))
		{
			world.worldMatrix = child.localTransform *
				em.GetComponent<MixamoHeadJointTF>(child.boneParent).transform *
				em.GetComponent<TransformComponent>(child.boneParent).worldMatrix;

			world.SetPosition(world.GetPosition() + Vector3(0.f, -0.5f, 0.f)); // Account for capsule offset
		}
	}
	else
	{
		em.DeferredEntityDestruction(e);
	}
}

void SetGunToBoneSystem::OnUpdate(DOG::entity e, ChildToBoneComponent& child, DOG::TransformComponent& world)
{
	auto& em = EntityManager::Get();
	if (em.Exists(child.boneParent))
	{
		if (em.HasComponent<ModelComponent>(e))
		{
			world.worldMatrix = child.localTransform *
				em.GetComponent<MixamoRightHandJointTF>(child.boneParent).transform *
				em.GetComponent<TransformComponent>(child.boneParent).worldMatrix;

			world.SetPosition(world.GetPosition() + Vector3(0.f, -0.5f, 0.f)); // Account for model capsule offset
		}
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

#pragma endregion