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
	IncreaseMaxHp,
	IncreaseSpeed,
	IncreaseSpeed2,
	JumpBoost,
	PassiveItems,
	//Health move to good spot when health is in
	//ActiveItem
	ActiveItemsBegin,
	Trampoline,
	Turret,
	ActiveItems,

	//Barrel
	BarrelItemsBegin,
	BulletBarrel,
	GrenadeBarrel,
	MissileBarrel,
	LaserBarrel,
	Barrels,

	//MagazineModification
	MagazineModificationItemsBegin,
	DefaultMagazineModification,
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
