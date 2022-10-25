#pragma once

#include <ostream>

enum class EntityTypes
{
	//Agents
	AgentsBegin,
	Scorpio = AgentsBegin,
	Agents,

	//Pickups
	PickupsBegin,
	Medkit = PickupsBegin,
	Pickups,

	//PassiveItem
	PassiveItemsBegin,
	IncreaseMaxHp = PassiveItemsBegin,
	PassiveItems,
	Default,
};

//std::ostream& operator<<(std::ostream& os, EntitesTypes& type)
//{
//	return os << static_cast<u32>(type);
//}

//u32 RangeCastEntitesTypes(EntitesTypes start, EntitesTypes type)
//{
//	return static_cast<u32>(type) - static_cast<u32>(start);
//}
