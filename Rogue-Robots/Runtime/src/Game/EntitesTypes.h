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

	//ActiveItem
	ActiveItemsBegin,
	Trampoline = ActiveItemsBegin,
	ActiveItems,

	//Barrel
	BarrelItemsBegin,
	BulletBarrel = BarrelItemsBegin,
	GrenadeBarrel,
	MissileBarrel,
	Barrels,

	//MagazineModification
	MagazineModificationItemsBegin,
	DefaultMagazineModification = MagazineModificationItemsBegin,
	FrostMagazineModification,
	Magazines,

	Default,
};
