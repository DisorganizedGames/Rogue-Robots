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
	IncreaseSpeed,
	IncreaseSpeed2,
	Health,
	JumpBoost,
	PassiveItems,

	//ActiveItem
	ActiveItemsBegin,
	Trampoline = ActiveItemsBegin,
	Turret,
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

	//Misc
	MiscItemsBegin,
	BasicMisc = MiscItemsBegin,
	FullAutoMisc,
	ChargeShotMisc,
	Miscs,

	SpawnAble,

	Default,
};
