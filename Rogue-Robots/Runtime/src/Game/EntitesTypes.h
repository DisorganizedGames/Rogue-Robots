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
