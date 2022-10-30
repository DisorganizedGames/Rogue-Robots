#include "QueryHelpers.h"
#include "EntityManager.h"
namespace DOG
{
	entity GetPlayer() noexcept
	{
		entity player = NULL_ENTITY;
		EntityManager::Get().Collect<ThisPlayer>().Do([&player](entity e, ThisPlayer&) { player = e; });
		return player;
	}
}