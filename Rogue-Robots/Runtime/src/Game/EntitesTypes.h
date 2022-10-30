#pragma once

#include <ostream>

enum class EntityTypes
{
	//Agents
	AgentsBegin,
	Scorpio = AgentsBegin,
	Scorpio1,
	Scorpio2,
	Scorpio3,
	Scorpio4,
	Scorpio5,
	Scorpio6,
	Scorpio7,
	Scorpio8,
	Scorpio9,
	Scorpio10,
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
