#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"


class ItemManager
{
	using Vector3 = DirectX::SimpleMath::Vector3;

private:


public:
	[[nodiscard]] static constexpr ItemManager& Get() noexcept
	{
		if (s_notInitialized)
			Initialize();
		return s_amInstance;
	}

	u32 CreateItem(EntityTypes itemType, Vector3 position, u32 id = 0);
	void CreateItemHost(EntityTypes itemType, Vector3 position);
	void CreateItemClient(CreateAndDestroyEntityComponent cad);
	void DestroyAllItems();
private:
	// singelton instance
	static ItemManager s_amInstance;
	static bool s_notInitialized;
	static DOG::EntityManager& s_entityManager;
	ItemManager() noexcept;
	~ItemManager() noexcept = default;
	DELETE_COPY_MOVE_CONSTRUCTOR(ItemManager);
	static void Initialize();

	u32 CreateTrampolinePickup(Vector3 position, u32 id = 0);
	u32 CreateMissilePickup(Vector3 position, u32 id = 0);
	u32 CreateLaserPickup(Vector3 position, u32 id = 0);
	u32 CreateGrenadePickup(Vector3 position, u32 id = 0);
	u32 CreateMaxHealthBoostPickup(Vector3 position, u32 id = 0);
	u32 CreateFrostModificationPickup(Vector3 position, u32 id = 0);
	u32 CreateFireModificationPickup(Vector3 position, u32 id = 0);
	u32 CreateTurretPickup(Vector3 position, u32 id = 0);
	u32 CreateSpeedBoostPickup(Vector3 position, u32 id = 0);
	u32 CreateSpeedBoost2Pickup(Vector3 position, u32 id = 0);
	u32 CreateHealthPickup(Vector3 position, u32 id = 0);
	u32 CreateJumpBoost(Vector3 position, u32 id = 0);
	u32 CreateFullAutoPickup(Vector3 position, u32 id = 0);
	u32 CreateChargeShotPickup(Vector3 position, u32 id = 0);
	u32 CreateReviverPickup(Vector3 position, u32 id = 0);
	u32 CreateGoalRadarPickup(Vector3 position, u32 id = 0);
	u32 CreateSyringePickup(Vector3 position, u32 id = 0);
};