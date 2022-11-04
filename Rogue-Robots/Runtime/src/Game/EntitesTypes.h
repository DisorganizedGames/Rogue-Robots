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

	//Barrel
	BarrelItemsBegin,
	BulletBarrel = BarrelItemsBegin,
	GrenadeBarrel,
	MissileBarrel,

	//MagazineModification
	MagazineModificationItemsBegin,
	DefaultMagazineModification = MagazineModificationItemsBegin,
	FrostMagazineModification,

	Default,
};
