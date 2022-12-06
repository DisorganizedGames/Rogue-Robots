#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"

class PlayerManager
{
	using Vector3 = DirectX::SimpleMath::Vector3;

public:
	[[nodiscard]] static constexpr PlayerManager& Get() noexcept
	{
		if (s_notInitialized)
			Initialize();
		return s_amInstance;
	}
	void HurtPlayer(DOG::entity player, f32 damage);
	void HurtThisPlayer(f32 damage);
	void HurtIfThisPlayer(DOG::entity player, f32 damage, DOG::entity byWho);
	DOG::entity GetThisPlayer();
	u8 GetNrOfPlayers();
	bool IsThisPlayerHost();
	bool IsThisMultiplayer();
	void HurtOnlinePlayers(DOG::entity player);
	i8 GetPlayerId(DOG::entity player);
private:
	// singelton instance
	static PlayerManager s_amInstance;
	static bool s_notInitialized;
	static DOG::EntityManager& s_entityManager;

	PlayerManager() noexcept;
	~PlayerManager() noexcept = default;
	DELETE_COPY_MOVE_CONSTRUCTOR(PlayerManager);
	static void Initialize();

private:
	void PlayHurtAudio(DOG::entity player);

private:
	u32 m_playerHurtAudio;
	u32 m_playerDeathAudio;
};

class PlayerHit : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::HasEnteredCollisionComponent, DOG::ThisPlayer);
	ON_UPDATE_ID(DOG::HasEnteredCollisionComponent, DOG::ThisPlayer);
	void OnUpdate(DOG::entity e, DOG::HasEnteredCollisionComponent& entered, DOG::ThisPlayer&);
};