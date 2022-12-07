#include "LuaInterfaces.h"
#include "GameComponent.h"
#include "ExplosionSystems.h"
#include "ItemManager/ItemManager.h"
#include "EntitesTypes.h"
#include "PlayerManager/PlayerManager.h"


using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;
//---------------------------------------------------------------------------------------------------------
//Input
void InputInterface::IsLeftPressed(LuaContext* context)
{
	context->ReturnBoolean(Mouse::IsButtonPressed(Button::Left));
}

void InputInterface::IsRightPressed(LuaContext* context)
{
	context->ReturnBoolean(Mouse::IsButtonPressed(Button::Right));
}

void InputInterface::IsKeyPressed(LuaContext* context)
{
	std::string input = context->GetString();
	context->ReturnBoolean(Keyboard::IsKeyPressed((Key)std::toupper(input[0]))); //Usch
}

void InputInterface::GetMouseDelta(DOG::LuaContext* context)
{
	auto [x, y] = DOG::Mouse::GetDeltaCoordinates();
	LuaTable t;
	t.AddIntToTable("x", x);
	t.AddIntToTable("y", y);
	context->ReturnTable(t);
};

//---------------------------------------------------------------------------------------------------------
//Entity
void EntityInterface::CreateEntity(LuaContext* context)
{
	context->ReturnInteger(EntityManager::Get().CreateEntity());
}

void EntityInterface::DestroyEntity(LuaContext* context)
{
	EntityManager::Get().DeferredEntityDestruction(context->GetInteger());
}

void EntityInterface::AddComponent(LuaContext* context)
{
	entity e = context->GetInteger();
	std::string compType = context->GetString();

	if (compType == "Model")
	{
		AddModel(context, e);
	}
	else if (compType == "Transform")
	{
		AddTransform(context, e);
	}
	else if (compType == "NetworkTransform")
	{
		AddNetwork(e);
	}
	else if (compType == "AgentStats")
	{
		AddAgentStats(context, e);
	}
	else if (compType == "Audio")
	{
		AddAudio(context, e);
	}
	else if (compType == "BoxCollider")
	{
		AddBoxCollider(context, e);
	}
	else if (compType == "BoxColliderMass")
	{
		AddBoxColliderMass(context, e);
	}
	else if (compType == "SphereCollider")
	{
		AddSphereCollider(context, e);
	}
	else if (compType == "SphereTrigger")
	{
		AddSphereTrigger(context, e);
	}
	else if (compType == "Rigidbody")
	{
		AddRigidbody(context, e);
	}
	else if (compType == "Script")
	{
		AddScript(context, e);
	}
	else if (compType == "Bullet")
	{
		AddBullet(context, e);
	}
	else if (compType == "FrostEffect")
	{
		EntityManager::Get().AddComponent<FrostEffectComponent>(e).frostTimer = (float)context->GetDouble();
	}
	else if (compType == "FireEffect")
	{
		AddFireEffectComponent(context, e);
	}
	else if (compType == "SubMeshRender")
	{
		AddSubmeshRender(context, e);
	}
	else if (compType == "HomingMissileComponent")
	{
		AddHomingMissile(context, e);
	}
	else if (compType == "ActiveItem")
	{
		AddActiveItem(context, e);
	}
	else if (compType == "BarrelComponent")
	{
		AddBarrelComponent(context, e);
	}
	else if (compType == "MagazineModificationComponent")
	{
		AddMagazineModificationComponent(context, e);
	}
	else if (compType == "MiscComponent")
	{
		AddMiscComponent(context, e);
	}
	else if (compType == "ThisPlayerWeapon")
	{
		AddThisPlayerWeapon(context, e);
	}
	else if (compType == "TurretTargeting")
	{
		AddTurretTargeting(context, e);
	}
	else if (compType == "TurretBasicShooting")
	{
		AddTurretBasicShooting(context, e);
	}
	else if (compType == "Child")
	{
		AddChildComponent(context, e);
	}
	else if (compType == "ShadowReciever")
	{
		AddShadowReciever(e);
	}
	else if (compType == "LaserBullet")
	{
		AddLaserBullet(context, e);
	}
	else if (compType == "PointLight")
	{
		AddPointLight(context, e);
	}
	else if (compType == "WeaponLight")
	{
		AddWeaponLightComponent(context, e);
	}
	else if (compType == "GoalRadarComponent")
	{
		AddGoalRadarComponent(context, e);
	}
	else if (compType == "OutlineComponent")
	{
		AddOutlineComponent(context, e);
	}
	else if (compType == "LifetimeComponent")
	{
		AddLifetimeComponent(context, e);
	}
	//Add more component types here.
	else
	{
		assert(false && "Lua can't create component");
	}
}

void EntityInterface::RemoveComponent(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	std::string compType = context->GetString();
	if (compType == "ActiveItem")
	{
		EntityManager::Get().RemoveComponent<ActiveItemComponent>(e);

		if (EntityManager::Get().HasComponent<ThisPlayer>(e))
		{
			DOG::UI::Get()->GetUI<UIIcon>(iconActiveID)->Hide();
		}
		return;
	}
	else if (compType == "BarrelComponent")
	{
		EntityManager::Get().RemoveComponent<BarrelComponent>(e);
		EntityManager::Get().RemoveComponentIfExists<LaserBarrelComponent>(e);
		EntityManager::Get().RemoveComponentIfExists<LaserBeamComponent>(e);
		if (auto vfxLaser = EntityManager::Get().TryGetComponent<LaserBeamVFXComponent>(e))
		{
			if (EntityManager::Get().Exists(vfxLaser->get().particleEmitter))
			{
				EntityManager::Get().DeferredEntityDestruction(vfxLaser->get().particleEmitter);
			}
			EntityManager::Get().RemoveComponent<LaserBeamVFXComponent>(e);
		}
		return;
	}
	else if (compType == "MagazineModificationComponent")
	{
		EntityManager::Get().RemoveComponent<MagazineModificationComponent>(e);
		return;
	}
	else if (compType == "MiscComponent")
	{
		EntityManager::Get().RemoveComponent<MiscComponent>(e);
		return;
	}
	else if (compType == "Model")
	{
		EntityManager::Get().RemoveComponent<ModelComponent>(e);
		EntityManager::Get().RemoveComponent<ShadowReceiverComponent>(e);
		return;
	}

	ASSERT(false, "Lua can't remove component");
}

void EntityInterface::ModifyComponent(LuaContext* context)
{
	entity e = context->GetInteger();
	std::string compType = context->GetString();

	if (compType == "Transform")
	{
		ModifyTransform(context, e);
	}
	else if (compType == "PlayerStats")
	{
		ModifyPlayerStats(context, e);
	}
	else if (compType == "PointLightStrength")
	{
		ModifyPointLightStrength(context, e);
	}
	else if (compType == "LaserBarrel")
	{
		ModifyLaserBarrel(context, e);
	}
	//Add more component types here.
}

void EntityInterface::GetTransformPosData(LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);

	DirectX::SimpleMath::Vector3 pos = transform.worldMatrix.Translation();

	LuaTable t;
	t.AddFloatToTable("x", pos.x);
	t.AddFloatToTable("y", pos.y);
	t.AddFloatToTable("z", pos.z);
	context->ReturnTable(t);
}

void EntityInterface::GetForward(LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	Matrix world = transform.worldMatrix;

	LuaTable t;
	t.AddFloatToTable("x", world._31);
	t.AddFloatToTable("y", world._32);
	t.AddFloatToTable("z", world._33);
	context->ReturnTable(t);
}

void EntityInterface::GetUp(LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	Matrix world = transform.worldMatrix;

	LuaTable t;
	t.AddFloatToTable("x", world._21);
	t.AddFloatToTable("y", world._22);
	t.AddFloatToTable("z", world._23);
	context->ReturnTable(t);
}

void EntityInterface::GetRight(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	Matrix world = transform.worldMatrix;

	LuaTable t;
	t.AddFloatToTable("x", world._11);
	t.AddFloatToTable("y", world._12);
	t.AddFloatToTable("z", world._13);
	context->ReturnTable(t);
}

void EntityInterface::GetPlayerControllerCamera(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto& player = EntityManager::Get().GetComponent<PlayerControllerComponent>(e);
	context->ReturnInteger(player.cameraEntity);
}

static bool GetActionState(InputController& input, const std::string& action)
{
	std::unordered_map<std::string, bool> map = {
		{"Shoot", input.shoot},
		{"Jump", input.jump},
		{"Forwards", input.forward},
		{"Backwards", input.backwards},
		{"Left", input.left},
		{"Right", input.right},
		{"ActivateItem", input.activateActiveItem},
		{"SwitchComponent", input.switchComp},
		{"SwitchBarrelComponent", input.switchBarrelComp},
		{"SwitchMagazineComponent", input.switchMagazineComp},
		{"ActivateActiveItem", input.activateActiveItem},
		{"Reload", input.reload},
	};

	return map.at(action);
}

//1. Shoot, 2. Jump, 3.activateActiveItem
void EntityInterface::GetAction(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	std::string action = context->GetString();
	InputController& input = EntityManager::Get().GetComponent<InputController>(e);

	context->ReturnBoolean(GetActionState(input, action));
}

void EntityInterface::SetAction(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	int action = context->GetInteger();
	bool active = context->GetBoolean();
	InputController& input = EntityManager::Get().GetComponent<InputController>(e);
	switch (action)
	{
	case 1:
		input.shoot = active;
		break;
	case 2:
		input.jump = active;
		break;
	case 3:
		input.activateActiveItem = active;
		break;
	case 4:
		input.switchComp = active;
		break;
	case 5:
		input.switchBarrelComp = active;
		break;
	default:
		break;
	}

}


void EntityInterface::Exists(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	context->ReturnBoolean(EntityManager::Get().Exists(e));
}

void EntityInterface::GetEntityTypeAsString(DOG::LuaContext* context)
{
	EntityTypes type = (EntityTypes)context->GetInteger();
	switch (type)
	{
	case EntityTypes::Trampoline:
		context->ReturnString("Trampoline");
		break;
	case EntityTypes::Turret:
		context->ReturnString("Turret");
		break;
	case EntityTypes::Reviver:
		context->ReturnString("Reviver");
		break;
	case EntityTypes::GoalRadar:
		context->ReturnString("GoalRadar");
		break;
	case EntityTypes::Syringe:
		context->ReturnString("Syringe");
		break;
	case EntityTypes::FrostMagazineModification:
		context->ReturnString("FrostMagazineModification");
		break;
	case EntityTypes::FireMagazineModification:
		context->ReturnString("FireMagazineModification");
		break;
	case EntityTypes::GrenadeBarrel:
		context->ReturnString("GrenadeBarrel");
		break;
	case EntityTypes::MissileBarrel:
		context->ReturnString("MissileBarrel");
		break;
	case EntityTypes::LaserBarrel:
		context->ReturnString("LaserBarrel");
		break;
	case EntityTypes::FullAutoMisc:
		context->ReturnString("FullAutoMisc");
		break;
	case EntityTypes::ChargeShotMisc:
		context->ReturnString("ChargeShotMisc");
		break;
	case EntityTypes::IncreaseMaxHp:
		context->ReturnString("MaxHealthBoost");
		break;
	case EntityTypes::IncreaseSpeed:
		context->ReturnString("SpeedBoost");
		break;
	case EntityTypes::IncreaseSpeed2:
		context->ReturnString("SpeedBoost2");
		break;
	//case EntityTypes::Health:
	//	context->ReturnString("Health");
	//	break;
	case EntityTypes::JumpBoost:
		context->ReturnString("JumpBoost");
		break;
	default:
		context->ReturnString("default");
		break;
	}
}

//void EntityInterface::AgentHit(DOG::LuaContext* context)
//{
//	entity e = context->GetInteger();
//	entity bullet = context->GetInteger();
//	assert(EntityManager::Get().HasComponent<BulletComponent>(bullet));
//	i8 playerNetworkID = EntityManager::Get().GetComponent<BulletComponent>(bullet).playerId;
//	if (!EntityManager::Get().HasComponent<AgentHitComponent>(e))
//		EntityManager::Get().AddComponent<AgentHitComponent>(e).HitBy(bullet, playerNetworkID);
//	else
//		EntityManager::Get().GetComponent<AgentHitComponent>(e).HitBy(bullet, playerNetworkID);
//}

void EntityInterface::IsBulletLocal(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	BulletComponent& bullet = EntityManager::Get().GetComponent<BulletComponent>(e);
	context->ReturnBoolean(EntityManager::Get().HasComponent<ThisPlayer>(bullet.playerEntityID));
}

#pragma region HasComponent

template<typename ComponentType>
static bool HasComp(entity e)
{
	return EntityManager::Get().HasComponent<ComponentType>(e);
}

const std::unordered_map<std::string, bool (*) (entity)> componentMap = {
	// Engine Types
	{ "Transform", HasComp<TransformComponent> },
	{ "Model", HasComp<ModelComponent> },
	{ "Audio", HasComp<AudioComponent>},
	{ "NetworkTransform", HasComp<NetworkTransform>},
	{ "NetworkAgentStats", HasComp<NetworkAgentStats>},
	{ "Rigidbody", HasComp<RigidbodyComponent>},
	{ "BoxCollider", HasComp<BoxColliderComponent>},

	// Game Types
	{ "Bullet", HasComp<BulletComponent>},
	{ "PlayerStats", HasComp<PlayerStatsComponent> },
	{ "PassiveItem", HasComp<PassiveItemComponent> },
	{ "ActiveItem", HasComp<ActiveItemComponent> },
	{ "ThisPlayer", HasComp<ThisPlayer> },
	{ "FrostEffect", HasComp<FrostEffectComponent> },
	{ "BarrelComponent", HasComp<BarrelComponent> },
	{ "MagazineModificationComponent", HasComp<MagazineModificationComponent> },
};

void EntityInterface::HasComponent(LuaContext* context)
{
	entity e = context->GetInteger();
	bool hasComp = componentMap.at(context->GetString())(e);
	context->ReturnBoolean(hasComp);
}

#pragma endregion

void EntityInterface::PlayAudio(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	u32 asset = (u32)context->GetInteger();
	bool is3D = context->GetBoolean();

	auto& comp = EntityManager::Get().GetComponent<AudioComponent>(e);
	comp.assetID = asset;
	comp.is3D = is3D;
	comp.shouldPlay = true;
}

void EntityInterface::StopAudio(DOG::LuaContext* context)
{
	entity e = context->GetInteger();

	auto& comp = EntityManager::Get().GetComponent<AudioComponent>(e);
	comp.shouldStop = true;
}

void EntityInterface::IsPlayingAudio(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto& comp = EntityManager::Get().GetComponent<AudioComponent>(e);
	context->ReturnBoolean(comp.playing);
}

const std::unordered_map<PassiveItemComponent::Type, std::string> passiveTypeMap = {
	{ PassiveItemComponent::Type::Template, "Template" },
	{ PassiveItemComponent::Type::MaxHealthBoost, "MaxHealthBoost" },
	{ PassiveItemComponent::Type::SpeedBoost, "SpeedBoost" },
	{ PassiveItemComponent::Type::LifeSteal, "LifeSteal" },
};

const std::unordered_map<ActiveItemComponent::Type, std::string> activeTypeMap = {
	{ ActiveItemComponent::Type::Trampoline, "Trampoline" },
	{ ActiveItemComponent::Type::Turret, "Turret" },
	{ ActiveItemComponent::Type::Reviver, "Reviver" },
	{ ActiveItemComponent::Type::GoalRadar, "GoalRadar" },
	{ ActiveItemComponent::Type::Syringe, "Syringe" },
};

const std::unordered_map<BarrelComponent::Type, std::string> barrelTypeMap = {
	{ BarrelComponent::Type::Bullet, "BulletBarrel"},
	{ BarrelComponent::Type::Missile, "MissileBarrel" },
	{ BarrelComponent::Type::Grenade, "GrenadeBarrel" },
	{ BarrelComponent::Type::Laser, "LaserBarrel" },
};

const std::unordered_map<MagazineModificationComponent::Type, std::string> modificationTypeMap = {
	{ MagazineModificationComponent::Type::None, "None" },
	{ MagazineModificationComponent::Type::Frost, "FrostMagazineModification"},
	{ MagazineModificationComponent::Type::Fire, "FireMagazineModification"},
};

const std::unordered_map<MiscComponent::Type, std::string> miscTypeMap = {
	{ MiscComponent::Type::Basic, "BasicMisc" },
	{ MiscComponent::Type::FullAuto, "FullAutoMisc"},
	{ MiscComponent::Type::ChargeShot, "ChargeShotMisc"},
};

void EntityInterface::GetPassiveType(LuaContext* context)
{
	entity e = context->GetInteger();
	auto type = EntityManager::Get().GetComponent<PassiveItemComponent>(e).type;
	context->ReturnString(passiveTypeMap.at(type));
}

void EntityInterface::GetActiveType(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto type = EntityManager::Get().GetComponent<ActiveItemComponent>(e).type;
	context->ReturnString(activeTypeMap.at(type));
}

void EntityInterface::GetBarrelType(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
 	auto type = EntityManager::Get().GetComponent<BarrelComponent>(e).type;
	context->ReturnString(barrelTypeMap.at(type));
}

void EntityInterface::GetModificationType(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto type = EntityManager::Get().GetComponent<MagazineModificationComponent>(e).type;
	context->ReturnString(modificationTypeMap.at(type));
}

void EntityInterface::GetMiscType(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto type = EntityManager::Get().GetComponent<MiscComponent>(e).type;
	context->ReturnString(miscTypeMap.at(type));
}

void EntityInterface::GetAmmoCapacityForBarrelType(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto capacity = EntityManager::Get().GetComponent<BarrelComponent>(e).maximumAmmoCapacityForType;
	context->ReturnInteger(capacity);
}

void EntityInterface::GetAmmoCountPerPickup(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	auto ammoPerPickup = EntityManager::Get().GetComponent<BarrelComponent>(e).ammoPerPickup;
	context->ReturnInteger(ammoPerPickup);
}

void EntityInterface::GetTransformScaleData(LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	DirectX::SimpleMath::Vector3 scale;
	DirectX::SimpleMath::Quaternion rot;
	DirectX::SimpleMath::Vector3 pos;
	transform.worldMatrix.Decompose(scale, rot, pos);

	LuaTable t;
	t.AddFloatToTable("x", scale.x);
	t.AddFloatToTable("y", scale.y);
	t.AddFloatToTable("z", scale.z);
	context->ReturnTable(t);
}

void EntityInterface::SetRotationForwardUp(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	LuaTable forwardTable = context->GetTable();
	LuaTable upTable = context->GetTable();
	LuaVector3 forwardLua(forwardTable);
	LuaVector3 upLua(upTable);

	Vector3 forward = ::XMVectorSet(forwardLua.x, forwardLua.y, forwardLua.z, 0);
	Vector3 up = ::XMVectorSet(upLua.x, upLua.y, upLua.z, 0);
	Vector3 right = ::XMVector3Cross(up, forward);

	/*Matrix rotMat = DirectX::XMMatrixLookToLH((DirectX::XMVECTOR)transform.GetPosition(), forward, up);
	rotMat = rotMat.Invert();*/
	Matrix rotMat = DirectX::XMMatrixLookToLH(Vector3::Zero, forward, up);
	rotMat = rotMat.Invert();

	transform.SetRotation(rotMat);
}

void EntityInterface::GetPlayerStats(LuaContext* context)
{
	entity e = context->GetInteger();

	PlayerStatsComponent& psComp = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);

	LuaTable t;
	t.AddDoubleToTable("health", psComp.health);
	t.AddDoubleToTable("maxHealth", psComp.maxHealth);
	t.AddDoubleToTable("speed", psComp.speed);
	t.AddDoubleToTable("lifeSteal", psComp.lifeSteal);
	t.AddDoubleToTable("jumpSpeed", psComp.jumpSpeed);
	context->ReturnTable(t);
}

void EntityInterface::GetPlayerStat(LuaContext* context)
{
	entity e = context->GetInteger();
	auto stat = context->GetString();
	
	PlayerStatsComponent& psComp = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);
	
	std::unordered_map<std::string, std::variant<f32>> statMap = {
		{ "health", psComp.health },
		{ "maxHealth", psComp.maxHealth },
		{ "speed", psComp.speed },
		{ "lifeSteal", psComp.lifeSteal },
		{ "jumpSpeed", psComp.jumpSpeed },
	};
	
	auto& out = statMap.at(stat);
	if (f32* val = std::get_if<f32>(&out))
	{
		context->ReturnDouble(*val);
	}
}

void EntityInterface::SetPlayerStats(LuaContext* context)
{
	entity e = context->GetInteger();
	auto stats = context->GetTable();
	EntityManager::Get().GetComponent<PlayerStatsComponent>(e) = {
		.maxHealth = stats.GetFloatFromTable("maxHealth"),
		.health = stats.GetFloatFromTable("health"),
		.speed = stats.GetFloatFromTable("speed"),
		.lifeSteal = stats.GetFloatFromTable("lifeSteal"),
		.jumpSpeed = stats.GetFloatFromTable("jumpSpeed"),
	};
}

void EntityInterface::SetPlayerStat(LuaContext* context) 
{
	entity e = context->GetInteger();
	auto stat = context->GetString();

	PlayerStatsComponent& psComp = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);

	std::unordered_map<std::string, std::variant<f32*>> statMap = {
		{ "health", &psComp.health },
		{ "maxHealth", &psComp.maxHealth },
		{ "speed", &psComp.speed },
		{ "lifeSteal", &psComp.lifeSteal },
		{ "jumpSpeed", &psComp.jumpSpeed },
	};

	auto& out = statMap.at(stat);
	if (f32** val = std::get_if<f32*>(&out))
	{
		*(*val) = static_cast<f32>(context->GetDouble());
		//std::cout << "New value: " << **val << std::endl;
	}
}

void EntityInterface::AddModel(LuaContext* context, entity e)
{
	EntityManager::Get().AddComponent<ModelComponent>(e, static_cast<u32>(std::stoull(context->GetString())));
	EntityManager::Get().AddComponent<ShadowReceiverComponent>(e);
}

void EntityInterface::AddTransform(LuaContext* context, entity e)
{
	LuaTable pos = context->GetTable();
	LuaTable rot = context->GetTable();
	LuaTable scale = context->GetTable();

	EntityManager::Get().AddComponent<TransformComponent>(e)
		.SetPosition({ pos.GetFloatFromTable("x"), pos.GetFloatFromTable("y") , pos.GetFloatFromTable("z") })
		.SetRotation({ rot.GetFloatFromTable("x"), rot.GetFloatFromTable("y") , rot.GetFloatFromTable("z") })
		.SetScale({ scale.GetFloatFromTable("x"), scale.GetFloatFromTable("y"), scale.GetFloatFromTable("z") });
}

void EntityInterface::AddNetwork(DOG::entity e)
{
	EntityManager::Get().AddComponent<NetworkTransform>(e);
}

void EntityInterface::AddAgentStats(LuaContext* context, entity e)
{
	LuaTable stats = context->GetTable();

	auto& agentStats = EntityManager::Get().AddComponent<AgentStatsComponent>(e);
	agentStats = {
		.hp = stats.GetFloatFromTable("hp"),
		.maxHP = stats.GetFloatFromTable("maxHP"),
		.speed = stats.GetFloatFromTable("speed"),
		.roomId = static_cast<u32>(stats.GetIntFromTable("roomId")),
	};
}

void EntityInterface::ModifyAnimationComponent(DOG::LuaContext* context)
{
	entity e = context->GetInteger();

	int animID = context->GetInteger();
	int group = context->GetInteger();
	float transitionLength = static_cast<f32>(context->GetDouble());
	float playbackRate = static_cast<f32>(context->GetDouble());

	if (!EntityManager::Get().HasComponent<AnimationComponent>(e))
		return;

	auto& aComp = EntityManager::Get().GetComponent<AnimationComponent>(e);

	auto& setter = aComp.animSetters[aComp.addedSetters++];
	setter.animationIDs[0] = static_cast<i8>(animID);
	setter.group = static_cast<u8>(group);
	setter.transitionLength = transitionLength;
	setter.playbackRate = playbackRate;
}

void EntityInterface::SpawnActiveItem(DOG::LuaContext* context)
{
	entity playerId = context->GetInteger();

	if (EntityManager::Get().GetComponent<ActiveItemComponent>(playerId).type == ActiveItemComponent::Type::Trampoline)
		ItemManager::Get().CreateItem(EntityTypes::Trampoline, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
	else if (EntityManager::Get().GetComponent<ActiveItemComponent>(playerId).type == ActiveItemComponent::Type::Turret)
		ItemManager::Get().CreateItem(EntityTypes::Turret, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
	else if (EntityManager::Get().GetComponent<ActiveItemComponent>(playerId).type == ActiveItemComponent::Type::Reviver)
		ItemManager::Get().CreateItem(EntityTypes::Reviver, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
	else if (EntityManager::Get().GetComponent<ActiveItemComponent>(playerId).type == ActiveItemComponent::Type::GoalRadar)
		ItemManager::Get().CreateItem(EntityTypes::GoalRadar, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
	else if (EntityManager::Get().GetComponent<ActiveItemComponent>(playerId).type == ActiveItemComponent::Type::Syringe)
		ItemManager::Get().CreateItem(EntityTypes::Syringe, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
}

void EntityInterface::GetOutlineColor(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	OutlineComponent& oc = EntityManager::Get().GetComponent<OutlineComponent>(e);
	LuaTable t;
	t.AddDoubleToTable("r", oc.color.x);
	t.AddDoubleToTable("g", oc.color.y);
	t.AddDoubleToTable("b", oc.color.z);
	context->ReturnTable(t);
}

void EntityInterface::AddAudio(LuaContext* context, entity e)
{
	auto assetID = context->GetInteger();
	auto shouldPlay = context->GetBoolean();
	auto is3D = context->GetBoolean();

	auto& comp = EntityManager::Get().AddComponent<AudioComponent>(e);

	comp.assetID = assetID;
	comp.shouldPlay = shouldPlay;
	comp.is3D = is3D;
}

void EntityInterface::AddBoxCollider(LuaContext* context, entity e)
{
	LuaTable boxDimTable = context->GetTable();
	bool dynamic = context->GetBoolean();

	LuaVector3 boxDim = LuaVector3(boxDimTable);

	EntityManager::Get().AddComponent<BoxColliderComponent>(e, e, Vector3{ boxDim.x, boxDim.y, boxDim.z }, dynamic);
}

void EntityInterface::AddBoxColliderMass(DOG::LuaContext* context, DOG::entity e)
{
	LuaTable boxDimTable = context->GetTable();
	bool dynamic = context->GetBoolean();

	float mass = (float)context->GetDouble();

	LuaVector3 boxDim = LuaVector3(boxDimTable);

	EntityManager::Get().AddComponent<BoxColliderComponent>(e, e, Vector3{ boxDim.x, boxDim.y, boxDim.z }, dynamic, mass);
}

void EntityInterface::AddSphereCollider(DOG::LuaContext* context, DOG::entity e)
{
	float radius = (float)context->GetDouble();
	bool dynamic = context->GetBoolean();

	EntityManager::Get().AddComponent<SphereColliderComponent>(e, e, radius, dynamic);
}

void EntityInterface::AddSphereTrigger(DOG::LuaContext* context, DOG::entity e)
{
	float radius = (float)context->GetDouble();

	EntityManager::Get().AddComponent<SphereTriggerComponent>(e, e, radius);
}

void EntityInterface::AddRigidbody(LuaContext* context, entity e)
{
	bool kinematic = context->GetBoolean();
	EntityManager::Get().AddComponent<RigidbodyComponent>(e, e, kinematic);
}

void EntityInterface::AddBullet(LuaContext* context, entity e)
{
	int playerEntity = context->GetInteger();
	BulletComponent& bullet = EntityManager::Get().AddComponent<BulletComponent>(e);
	bullet.playerEntityID = playerEntity;
	bullet.damage = static_cast<f32>(context->GetDouble());
	if (EntityManager::Get().HasComponent<RigidbodyComponent>(e))
		EntityManager::Get().GetComponent<RigidbodyComponent>(e).continuousCollisionDetection = true;
}

void EntityInterface::AddSubmeshRender(LuaContext* context, entity e)
{
	if (EntityManager::Get().HasComponent<ModelComponent>(e))
	{

		//Get material information from table
		LuaTable materialTable = context->GetTable();

		ModelComponent& model = EntityManager::Get().GetComponent<ModelComponent>(e);

		ModelAsset* modelAsset = AssetManager::Get().GetAsset<ModelAsset>(model.id);

		if (!modelAsset)
		{
			std::cout << "Model has not been loaded in yet! Model ID: " << model.id << "\n";
			return;
		}

		MaterialHandle materialHandle;
		materialHandle.handle = static_cast<u64>(materialTable.GetIntFromTable("materialHandle"));

		LuaTable albedoFactor = materialTable.GetTableFromTable("albedoFactor");
		LuaVector3 albedoFactorVector(albedoFactor);

		MaterialDesc materialDesc{};
		materialDesc.albedoFactor = { albedoFactorVector.x, albedoFactorVector.y, albedoFactorVector.z };
		materialDesc.roughnessFactor = (float)materialTable.GetDoubleFromTable("roughnessFactor");
		materialDesc.metallicFactor = (float)materialTable.GetDoubleFromTable("metallicFactor");

		EntityManager::Get().AddComponent<SubmeshRenderer>(e, modelAsset->gfxModel->mesh.mesh, materialHandle, materialDesc);
		EntityManager::Get().RemoveComponent<ModelComponent>(e);
	}
}

void EntityInterface::AddScript(DOG::LuaContext* context, DOG::entity e)
{
	LuaMain::GetScriptManager()->AddScript(e, context->GetString());
}

void EntityInterface::AddPointLight(DOG::LuaContext* context, DOG::entity e)
{
	LuaTable color = context->GetTable();
	float strength = (float)context->GetDouble();
	float radius = (float)context->GetDouble();

	// Add dynamic point light
	auto pdesc = PointLightDesc();
	pdesc.color = {color.GetFloatFromTable("x"), color.GetFloatFromTable("y"), color.GetFloatFromTable("z")};
	pdesc.strength = strength;
	pdesc.radius = radius;
	auto& plc = EntityManager::Get().AddComponent<PointLightComponent>(e);
	plc.handle = LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::PerFrame);
	plc.color = pdesc.color;
	plc.strength = pdesc.strength;
	plc.radius = pdesc.radius;
}

void EntityInterface::AddWeaponLightComponent(DOG::LuaContext*, DOG::entity e)
{
	EntityManager::Get().AddComponent<WeaponLightComponent>(e);
}

void EntityInterface::AddLifetimeComponent(DOG::LuaContext* context, DOG::entity e)
{
	float lifetime = (float)context->GetDouble();
	EntityManager::Get().AddComponent<LifetimeComponent>(e, lifetime);
}

void EntityInterface::AddHomingMissile(DOG::LuaContext* context, DOG::entity e)
{
	auto& em = EntityManager::Get();
	entity owner = context->GetInteger();
	assert(em.Exists(owner) && em.Exists(e));

	entity gun = context->GetInteger();
	assert(em.Exists(gun));

	LuaTable homingMissileInfo = context->GetTable();

	float explosionRadiusChange = 0.0f;
	float dmgChange = 0.0f;
	float startMotorSpeed = 0.0f;
	float mainMotorSpeed = 0.0f;
	float turnSpeed = 0.0f;
	float engineStartTime = 0.0f;
	float attackFlightPhaseStartTime = 0.0f;
	bool homing = false;

	homingMissileInfo.TryGetValueFromTable("explosionRadius", explosionRadiusChange);
	homingMissileInfo.TryGetValueFromTable("dmg", dmgChange);

	assert(!EntityManager::Get().HasComponent<HomingMissileComponent>(e));
	auto& missile = em.AddComponent<HomingMissileComponent>(e);
	missile.playerEntityID = owner;
	missile.explosionRadius *= explosionRadiusChange;
	missile.dmg *= dmgChange;

	if (homingMissileInfo.TryGetValueFromTable("startMotorSpeed", startMotorSpeed))
		missile.startMotorSpeed = startMotorSpeed;

	if (homingMissileInfo.TryGetValueFromTable("mainMotorSpeed", mainMotorSpeed))
		missile.mainMotorSpeed = mainMotorSpeed;

	if (homingMissileInfo.TryGetValueFromTable("turnSpeed", turnSpeed))
		missile.turnSpeed = turnSpeed;

	if (homingMissileInfo.TryGetValueFromTable("engineStartTime", engineStartTime))
		missile.engineStartTime = engineStartTime;

	if (homingMissileInfo.TryGetValueFromTable("attackFlightPhaseStartTime", attackFlightPhaseStartTime))
		missile.attackFlightPhaseStartTime = attackFlightPhaseStartTime;

	if (homingMissileInfo.TryGetValueFromTable("homing", homing))
		missile.homing = homing;

	assert(em.HasComponent<TransformComponent>(e));
	auto& t = em.GetComponent<TransformComponent>(e);
	auto& gunT = em.GetComponent<TransformComponent>(gun);
	Vector3 oldPosition = t.GetPosition();
	t.SetRotation(gunT.GetRotation() * Matrix::CreateFromAxisAngle(gunT.GetUp(), DirectX::XM_PI / 2.0f));
	Vector3 forward = t.GetForward();
	forward.Normalize();
	t.SetPosition(oldPosition + 0.5f * forward);

	em.AddComponent<BoxColliderComponent>(e, e, Vector3(0.18f, 0.18f, 0.8f), true, 12.0f);
	auto& rb = em.AddComponent<RigidbodyComponent>(e, e);
	rb.continuousCollisionDetection = true;

	if(em.HasComponent<RigidbodyComponent>(owner))
		DOG::PhysicsEngine::SetIgnoreCollisionCheck(rb.rigidbodyHandle, em.GetComponent<RigidbodyComponent>(owner).rigidbodyHandle, true);
}

void EntityInterface::ModifyTransform(LuaContext* context, entity e)
{
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	LuaTable t = context->GetTable();
	switch (context->GetInteger())
	{
	case 1:
		transform.SetPosition({ t.GetFloatFromTable("x"), t.GetFloatFromTable("y"), t.GetFloatFromTable("z") });
		break;
	case 2:
		transform.SetRotation({ t.GetFloatFromTable("x"), t.GetFloatFromTable("y") , t.GetFloatFromTable("z") });
		break;
	case 3:
		transform.SetScale({ t.GetFloatFromTable("x"), t.GetFloatFromTable("y") , t.GetFloatFromTable("z") });
		break;
	default:
		break;
	}
}

void EntityInterface::ModifyPlayerStats(DOG::LuaContext* context, DOG::entity e)
{
	auto t = context->GetTable();
	auto& psComp = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);

	psComp.health = t.GetFloatFromTable("health");
	psComp.maxHealth = t.GetFloatFromTable("maxHealth");
	psComp.speed = t.GetFloatFromTable("speed");
}

void EntityInterface::ModifyPointLightStrength(DOG::LuaContext* context, DOG::entity e)
{
	auto& c = EntityManager::Get().GetComponent<PointLightComponent>(e);
	c.dirty = true;
	float newStrength = (f32)context->GetDouble();
	c.radius *= newStrength / c.strength;
	c.strength = newStrength;
}

void EntityInterface::AddActiveItem(DOG::LuaContext* context, DOG::entity e)
{
	ActiveItemComponent::Type type = (ActiveItemComponent::Type)context->GetInteger();
	EntityManager::Get().AddComponent<ActiveItemComponent>(e).type = type;

	//Switch to correct UI icon.
	if (EntityManager::Get().HasComponent<ThisPlayer>(e))
	{
		DOG::UI::Get()->GetUI<UIIcon>(iconActiveID)->Show((UINT)type);
	}
}

void EntityInterface::AddBarrelComponent(DOG::LuaContext* context, DOG::entity e)
{
	BarrelComponent::Type type = (BarrelComponent::Type)context->GetInteger();
	auto currentAmmo = context->GetInteger();
	auto ammoCap = context->GetInteger();

	auto& bc = EntityManager::Get().AddComponent<BarrelComponent>(e);
	bc.type = type;
	bc.maximumAmmoCapacityForType = ammoCap;
	bc.currentAmmoCount = currentAmmo;
	bc.ammoPerPickup = currentAmmo;

	if (type == BarrelComponent::Type::Laser)
	{
		assert(!EntityManager::Get().HasComponent<LaserBarrelComponent>(e));
		EntityManager::Get().AddComponent<LaserBarrelComponent>(e).ammo = static_cast<f32>(currentAmmo);
	}

	//Switch to correct UI icon.
	if (EntityManager::Get().HasComponent<ThisPlayer>(e))
	{
		if ((UINT)type != 0)
		{
			DOG::UI::Get()->GetUI<UIIcon>(icon2ID)->Show((UINT)type - 1u);
		}
		else
		{
			DOG::UI::Get()->GetUI<UIIcon>(icon2ID)->Hide();
		}
	}
	
}

void EntityInterface::AddMagazineModificationComponent(DOG::LuaContext* context, DOG::entity e)
{
	MagazineModificationComponent::Type type = (MagazineModificationComponent::Type)context->GetInteger();

	auto& mc = EntityManager::Get().AddComponent<MagazineModificationComponent>(e);
	mc.type = type;

	//Switch to correct UI icon.
	if (EntityManager::Get().HasComponent<ThisPlayer>(e))
	{
		if ((UINT)type != 0)
		{
			DOG::UI::Get()->GetUI<UIIcon>(icon3ID)->Show((UINT)type - 1u);
		}
		else
		{
			DOG::UI::Get()->GetUI<UIIcon>(icon3ID)->Hide();
		}
	}
}

void EntityInterface::AddMiscComponent(DOG::LuaContext* context, DOG::entity e)
{
	MiscComponent::Type type = (MiscComponent::Type)context->GetInteger();

	auto& mmc = EntityManager::Get().AddComponent<MiscComponent>(e);
	mmc.type = type;

	//Switch to correct UI icon.
	if (EntityManager::Get().HasComponent<ThisPlayer>(e))
	{
		if ((UINT)type != 0)
		{
			DOG::UI::Get()->GetUI<UIIcon>(iconID)->Show((UINT)type - 1u);
		}
		else
		{
			DOG::UI::Get()->GetUI<UIIcon>(iconID)->Hide();
		}
	}
}

void EntityInterface::AddThisPlayerWeapon(DOG::LuaContext* context, DOG::entity e)
{
	UNREFERENCED_PARAMETER(context);
	// Simply add tag
	EntityManager::Get().AddComponent<ThisPlayerWeapon>(e);
}

void EntityInterface::AddTurretTargeting(DOG::LuaContext* context, DOG::entity e)
{
	auto& em = EntityManager::Get();
	assert(em.Exists(e));

	auto& targeter = em.AddComponent<TurretTargetingComponent>(e);
	targeter.maxRange = static_cast<float>(context->GetDouble());
	targeter.yawSpeed = static_cast<float>(context->GetDouble());
	targeter.pitchSpeed = static_cast<float>(context->GetDouble());
	targeter.yawLimit = static_cast<float>(context->GetDouble());
	targeter.pitchLimit = static_cast<float>(context->GetDouble());
}

void EntityInterface::AddTurretBasicShooting(DOG::LuaContext* context, DOG::entity e)
{
	auto& em = EntityManager::Get();

	entity owner = context->GetInteger(); // The player who spawned the turret.
	assert(em.Exists(owner) && em.Exists(e));

	auto& shooting = em.AddComponent<TurretBasicShootingComponent>(e);
	shooting.owningPlayer = owner;
	shooting.ammoCount = context->GetInteger();
	shooting.projectileSpeed = static_cast<float>(context->GetDouble());
	shooting.timeStep = context->GetDouble();
	shooting.damage = static_cast<float>(context->GetDouble());
	shooting.projectileLifeTime = static_cast<float>(context->GetDouble());
}

void EntityInterface::AddChildComponent(DOG::LuaContext* context, DOG::entity e)
{
	auto& em = EntityManager::Get();
	assert(em.Exists(e));

	entity parent = context->GetInteger();
	assert(em.Exists(parent) && em.HasComponent<TransformComponent>(parent));

	LuaTable pos = context->GetTable();
	LuaTable rot = context->GetTable();
	LuaTable scale = context->GetTable();

	auto& node = em.AddOrReplaceComponent<ChildComponent>(e);
	node.parent = parent;
	node.localTransform.SetPosition({ pos.GetFloatFromTable("x"), pos.GetFloatFromTable("y") , pos.GetFloatFromTable("z") })
		.SetRotation({ rot.GetFloatFromTable("x"), rot.GetFloatFromTable("y") , rot.GetFloatFromTable("z") })
		.SetScale({ scale.GetFloatFromTable("x"), scale.GetFloatFromTable("y"), scale.GetFloatFromTable("z") });
}

void EntityInterface::AddShadowReciever(DOG::entity e)
{
	auto& em = EntityManager::Get();
	assert(em.Exists(e));
	em.AddComponent<ShadowReceiverComponent>(e);
}

void EntityInterface::ModifyLaserBarrel(DOG::LuaContext* context, DOG::entity e)
{
	auto& em = EntityManager::Get();
	assert(em.Exists(e) && em.HasComponent<LaserBarrelComponent>(e));

	auto& barrel = em.GetComponent<LaserBarrelComponent>(e);
	barrel.laserToShoot.owningPlayer = context->GetInteger();
	barrel.shoot = context->GetBoolean();
	barrel.laserToShoot.maxRange = static_cast<f32>(context->GetDouble());
	barrel.damagePerSecond = static_cast<f32>(context->GetDouble());

	LuaTable laserStartTable = context->GetTable();
	LuaTable laserDirTable = context->GetTable();
	LuaTable laserColorTable = context->GetTable();
	LuaVector3 laserStart = LuaVector3(laserStartTable);
	LuaVector3 laserDir = LuaVector3(laserDirTable);
	LuaVector3 laserColor = LuaVector3(laserColorTable);

	barrel.laserToShoot.startPos = { laserStart.x, laserStart.y, laserStart.z };
	barrel.laserToShoot.direction = { laserDir.x, laserDir.y, laserDir.z };
	barrel.laserToShoot.color = { laserColor.x, laserColor.y, laserColor.z };


	if (auto mag = em.TryGetComponent<MagazineModificationComponent>(e))
	{
		if (mag->get().type == MagazineModificationComponent::Type::Frost)
		{
			em.AddOrReplaceComponent<FrostEffectComponent>(e).frostTimer = 3.5f;
		}
		else if (mag->get().type != MagazineModificationComponent::Type::Frost)
		{
			em.RemoveComponentIfExists<FrostEffectComponent>(e);
		}
		if (mag->get().type == MagazineModificationComponent::Type::Fire)
		{
			em.AddOrReplaceComponent<FireEffectComponent>(e, 1.0f, 15.0f, NULL_ENTITY, e);
		}
		else if (mag->get().type != MagazineModificationComponent::Type::Fire)
		{
			em.RemoveComponentIfExists<FireEffectComponent>(e);
		}
	}

	if (auto mag = em.TryGetComponent<BarrelComponent>(e))
	{
		mag->get().currentAmmoCount = static_cast<u32>(barrel.ammo);
	}

	// Signal true to lua to remove the component if out of ammo.
	bool outOfAmmo = barrel.ammo <= 0;
	if (outOfAmmo)
	{
		em.RemoveComponentIfExists<FrostEffectComponent>(e);
	}
	context->ReturnBoolean(barrel.ammo <= 0);
}

void EntityInterface::AddLaserBullet(DOG::LuaContext* context, DOG::entity e)
{
	entity owningPlayer = context->GetInteger();

	auto& em = EntityManager::Get();
	assert(em.Exists(e) && em.Exists(owningPlayer));
	MagazineModificationComponent::Type magType = MagazineModificationComponent::Type::None;
	if (auto mag = em.TryGetComponent<MagazineModificationComponent>(owningPlayer)) magType = mag->get().type;

	static std::optional<SubmeshRenderer> laserBulletModel = std::nullopt;
	if (!laserBulletModel)
	{
		MaterialDesc frostMat;
		frostMat.emissiveFactor = 6 * Vector4(1.5f, 0.1f, 0.1f, 0);
		frostMat.albedoFactor = { 0.5f, 0, 0, 1 };
		TransformComponent matrix;
		matrix.RotateW({ 0, 0, XM_PIDIV2 }).SetScale(Vector3(0.08f, 0.6f, 0.08f));
		laserBulletModel = CreateSimpleModel(frostMat, ShapeCreator(Shape::prism, 16, 8).GetResult()->mesh, matrix);
	}


	static std::optional<SubmeshRenderer> frostLaserBulletModel = std::nullopt;
	if (!frostLaserBulletModel)
	{
		MaterialDesc frostMat;
		frostMat.emissiveFactor = 6 * Vector4(0.2f, 0.8f, 0.8f, 1);
		frostMat.albedoFactor = { 0, 0, 0.5f, 1 };
		TransformComponent matrix;
		matrix.RotateW({ 0, 0, XM_PIDIV2 }).SetScale(Vector3(0.08f, 0.6f, 0.08f));
		frostLaserBulletModel = CreateSimpleModel(frostMat, ShapeCreator(Shape::prism, 16, 8).GetResult()->mesh, matrix);
	}

	static std::optional<SubmeshRenderer> fireLaserBulletModel = std::nullopt;
	if (!fireLaserBulletModel)
	{
		MaterialDesc fireMat;
		fireMat.emissiveFactor = 6 * Vector4(1.4f, 0.15f, 0.05f, 1);
		fireMat.albedoFactor = { 1.0f, 0.4f, 0.0f, 1 };
		TransformComponent matrix;
		matrix.RotateW({ 0, 0, XM_PIDIV2 }).SetScale(Vector3(0.08f, 0.6f, 0.08f));
		fireLaserBulletModel = CreateSimpleModel(fireMat, ShapeCreator(Shape::prism, 16, 8).GetResult()->mesh, matrix);
	}

	Vector3 color;
	switch (magType)
	{
	case MagazineModificationComponent::Type::None:
		em.AddComponent<SubmeshRenderer>(e) = *laserBulletModel;
		color.x = laserBulletModel->materialDesc.emissiveFactor.x;
		color.y = laserBulletModel->materialDesc.emissiveFactor.y;
		color.z = laserBulletModel->materialDesc.emissiveFactor.z;
		break;
	case MagazineModificationComponent::Type::Frost:
		em.AddComponent<SubmeshRenderer>(e) = *frostLaserBulletModel;
		color.x = frostLaserBulletModel->materialDesc.emissiveFactor.x;
		color.y = frostLaserBulletModel->materialDesc.emissiveFactor.y;
		color.z = frostLaserBulletModel->materialDesc.emissiveFactor.z;
		break;
	case MagazineModificationComponent::Type::Fire:
		em.AddComponent<SubmeshRenderer>(e) = *fireLaserBulletModel;
		color.x = fireLaserBulletModel->materialDesc.emissiveFactor.x;
		color.y = fireLaserBulletModel->materialDesc.emissiveFactor.y;
		color.z = fireLaserBulletModel->materialDesc.emissiveFactor.z;
		break;
	default:
		break;
	}

	em.AddComponent<LaserBulletComponent>(e).color = color;
}

void EntityInterface::AddGoalRadarComponent(DOG::LuaContext* context, DOG::entity e)
{
	f32 goalRadarTime = (f32)context->GetDouble();
	EntityManager::Get().AddComponent<GoalRadarComponent>(e, goalRadarTime);
}

void EntityInterface::AddFireEffectComponent(DOG::LuaContext* context, DOG::entity e)
{
	entity player = context->GetInteger();
	f32 fireEffectTime = (f32)context->GetDouble();
	f32 damagePerTick = (f32)context->GetDouble();

	EntityManager::Get().AddComponent<FireEffectComponent>(e, fireEffectTime, damagePerTick).playerEntityID = player;
}

void EntityInterface::AddOutlineComponent(DOG::LuaContext* context, DOG::entity e)
{
	f32 r = (f32)context->GetDouble();
	f32 g = (f32)context->GetDouble();
	f32 b = (f32)context->GetDouble();
	EntityManager::Get().AddComponent<OutlineComponent>(e).color = { r, g, b };
}



void EntityInterface::UpdateMagazine(DOG::LuaContext* context)
{
	entity player = context->GetInteger();
	auto currentMagazineCount = context->GetInteger();
	EntityManager::Get().GetComponent<BarrelComponent>(player).currentAmmoCount = currentMagazineCount;
}

//---------------------------------------------------------------------------------------------------------
//Asset

void AssetInterface::LoadModel(LuaContext* context)
{
	//Send string? Cast to double? Change asset manager to use 32bit?
	context->ReturnString(std::to_string(AssetManager::Get().LoadModelAsset(context->GetString())));
}

void AssetInterface::LoadAudio(LuaContext* context)
{
	auto filePath = context->GetString();
	auto audioAssetID = AssetManager::Get().LoadAudio(filePath, DOG::AssetLoadFlag::CPUMemory);
	context->ReturnInteger((i32)audioAssetID); // Pray we don't have over 2 billion assets
}

//-----------------------------------------------------------------------------------------wwwwwwwwwwwwww----------------
//Host
void HostInterface::DistanceToPlayers(DOG::LuaContext* context)
{
	struct PlayerDist
	{
		entity entityID;
		int playerID;
		Vector3 pos;
		float dist;
		PlayerDist(entity eID, int pID, Vector3 p, float d) : entityID(eID), playerID(pID), pos(p), dist(d) {}
		bool operator<(PlayerDist& o) { return dist < o.dist; }
	};
	LuaTable t = context->GetTable();
	Vector3 agentPos = Vector3(
		t.GetFloatFromTable("x"),
		t.GetFloatFromTable("y"),
		t.GetFloatFromTable("z")
	);
	std::vector<PlayerDist> distances;
	distances.reserve(4);

	EntityManager::Get().Collect<ThisPlayer, TransformComponent, NetworkPlayerComponent>().Do(
		[&](entity id, ThisPlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer)
		{
			distances.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
		});

	EntityManager::Get().Collect<OnlinePlayer, TransformComponent, NetworkPlayerComponent>().Do(
		[&](entity id, OnlinePlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer)
		{
			distances.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
		});

	std::sort(distances.begin(), distances.end());

	LuaTable tbl;
	for (size_t i = 0; i < distances.size(); ++i)
	{
		LuaTable d;
		d.AddIntToTable("entityID", static_cast<int>(distances[i].entityID));
		d.AddIntToTable("playerID", static_cast<int>(distances[i].playerID));
		d.AddTableToTable("pos", LuaVector3::Create(distances[i].pos));
		d.AddFloatToTable("dist", distances[i].dist);
		tbl.AddTableToTable(static_cast<int>(i), d);
	}
	context->ReturnTable(tbl);
}

void PhysicsInterface::RBSetVelocity(LuaContext* context)
{
	entity e = static_cast<u64>(context->GetInteger());
	LuaTable t = context->GetTable();
	LuaVector3 vel(t);
	auto& rigid = EntityManager::Get().GetComponent<RigidbodyComponent>(e);
	rigid.linearVelocity = { vel.x, vel.y, vel.z };
}

void PhysicsInterface::Explosion(DOG::LuaContext* context)
{
	entity explosionEntity = static_cast<u64>(context->GetInteger());

	float power = (float)context->GetDouble();
	float radius = (float)context->GetDouble();

	EntityManager::Get().AddComponent<ExplosionComponent>(explosionEntity, power, radius);
}

void PhysicsInterface::RBConstrainRotation(DOG::LuaContext* context)
{
	entity e = context->GetInteger();

	EntityManager::Get().GetComponent<RigidbodyComponent>(e).ConstrainRotation(context->GetBoolean(), context->GetBoolean(), context->GetBoolean());
}

void PhysicsInterface::RBConstrainPosition(DOG::LuaContext* context)
{
	entity e = context->GetInteger();

	EntityManager::Get().GetComponent<RigidbodyComponent>(e).ConstrainPosition(context->GetBoolean(), context->GetBoolean(), context->GetBoolean());
}

void PhysicsInterface::RayCast(DOG::LuaContext* context)
{
	LuaTable originTable = context->GetTable();
	LuaTable targetTable = context->GetTable();

	LuaVector3 origin = LuaVector3(originTable);
	LuaVector3 target = LuaVector3(targetTable);

	auto result = PhysicsEngine::RayCast({ origin.x, origin.y, origin.z }, { target.x, target.y, target.z });

	context->ReturnBoolean(result.operator bool());
	LuaTable r = LuaVector3::Create(result ? result->hitPosition : Vector3::Zero);
	context->ReturnTable(r);
}

LuaVector3::LuaVector3(LuaTable& table)
{
	x = table.GetFloatFromTable("x");
	y = table.GetFloatFromTable("y");
	z = table.GetFloatFromTable("z");
}

LuaTable LuaVector3::Create(Vector3 vec)
{
	LuaTable tbl;
	tbl.AddFloatToTable("x", vec.x);
	tbl.AddFloatToTable("y", vec.y);
	tbl.AddFloatToTable("z", vec.z);
	return tbl;
}

void SceneInterface::CreateEntity(LuaContext* context)
{
	entity e = context->GetInteger();
	assert(EntityManager::Get().Exists(e));
	entity newEntity = EntityManager::Get().CreateEntity();
	if (EntityManager::Get().HasComponent<SceneComponent>(e))
		EntityManager::Get().AddComponent<SceneComponent>(newEntity, EntityManager::Get().GetComponent<SceneComponent>(e).scene);
	context->ReturnInteger(newEntity);
}

void RenderInterface::CreateMaterial(DOG::LuaContext* context)
{
	//Create a new table and push the data to lua
	LuaTable table = context->GetTable();
	float roughnessFactor = (float)context->GetDouble();
	float metallicFactor = (float)context->GetDouble();

	LuaTable tab = context->GetTable();
	DirectX::SimpleMath::Vector4 emissiveFactor = { (float)tab.GetDoubleFromTable(0), (float)tab.GetDoubleFromTable(1), (float)tab.GetDoubleFromTable(2), 1.f };

	MaterialDesc d{};
	d.albedoFactor = { (float)table.GetDoubleFromTable(0), (float)table.GetDoubleFromTable(1), (float)table.GetDoubleFromTable(2), 1.f };
	d.roughnessFactor = roughnessFactor;
	d.metallicFactor = metallicFactor;
	d.emissiveFactor = emissiveFactor;
	auto mat = CustomMaterialManager::Get().AddMaterial(d);

	LuaTable material;
	material.AddIntToTable("materialHandle", (int)mat.handle);

	LuaTable returnAlbedoFactor;
	returnAlbedoFactor.AddFloatToTable("x", d.albedoFactor.x);
	returnAlbedoFactor.AddFloatToTable("y", d.albedoFactor.y);
	returnAlbedoFactor.AddFloatToTable("z", d.albedoFactor.z);
	material.AddTableToTable("albedoFactor", returnAlbedoFactor);

	material.AddFloatToTable("roughnessFactor", d.roughnessFactor);
	material.AddFloatToTable("metallicFactor", d.metallicFactor);

	LuaTable emissiveFactorTable;
	emissiveFactorTable.AddFloatToTable("x", d.emissiveFactor.x);
	emissiveFactorTable.AddFloatToTable("y", d.emissiveFactor.y);
	emissiveFactorTable.AddFloatToTable("z", d.emissiveFactor.z);
	material.AddTableToTable("emissiveFactor", emissiveFactorTable);

	context->ReturnTable(material);
}

void GameInterface::ExplosionEffect(DOG::LuaContext* context)
{
	auto entity = context->GetInteger();
	f32 radius = (f32)context->GetDouble();

	context->ReturnInteger(ExplosionEffectSystem::CreateExplosionEffect(entity, radius));
}

void GameInterface::AmmoUI(DOG::LuaContext* context)
{
	ImVec2 size;
	size.x = 280;
	size.y = 20;

	auto r = Window::GetWindowRect();
	ImVec2 pos;
	pos.x = r.right - size.x - 20.0f;
	pos.y = r.bottom - 50.0f;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Ammo", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
	{
		if (ImGui::BeginTable("Ammo", 2))
		{
			//ImGui::PushFont(ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/Fonts/Robot Radicals.ttf", 18.0f));
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
			ImGui::Text("Ammo:");
			ImGui::TableSetColumnIndex(1);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));

			//Set ammo text
			std::string currentAmmoText = std::to_string((long long)context->GetInteger());
			int ammoLeft = context->GetInteger();
			std::string ammoLeftText = "INF";
			if (ammoLeft > -1)
				ammoLeftText = std::to_string((long long)ammoLeft);
			std::string ammoText = currentAmmoText + " / " + ammoLeftText;
			ImGui::Text(ammoText.c_str());

			ImGui::PopStyleColor(2);
			//ImGui::PopFont();
			ImGui::EndTable();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
}

void GameInterface::AddDamageToEntity(DOG::LuaContext* context)
{
	entity e = (entity)context->GetInteger();
	int playerEntity = context->GetInteger();
	f32 damage = (f32)context->GetDouble();

	BulletComponent& bullet = EntityManager::Get().AddComponent<BulletComponent>(e);
	bullet.playerEntityID = playerEntity;
	bullet.damage = damage;


}

void GameInterface::AddMagazineEffectsFromBullet(DOG::LuaContext* context)
{
	entity bullet = context->GetInteger();
	entity newEntity = context->GetInteger();

	if (EntityManager::Get().HasComponent<FrostEffectComponent>(bullet))
	{
		EntityManager::Get().AddComponent<FrostEffectComponent>(newEntity) = EntityManager::Get().GetComponent<FrostEffectComponent>(bullet);
	}
	if (EntityManager::Get().HasComponent<FireEffectComponent>(bullet))
	{
		EntityManager::Get().AddComponent<FireEffectComponent>(newEntity) = EntityManager::Get().GetComponent<FireEffectComponent>(bullet);
	}
}

void GameInterface::SpawnPickupMiscComponent(DOG::LuaContext* context)
{
	entity playerId = context->GetInteger();
	MiscComponent::Type type = EntityManager::Get().GetComponent<MiscComponent>(playerId).type;

	if (type == MiscComponent::Type::ChargeShot)
	{
		ItemManager::Get().CreateItem(EntityTypes::ChargeShotMisc, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
	}
	else if (type == MiscComponent::Type::FullAuto)
	{
		ItemManager::Get().CreateItem(EntityTypes::FullAutoMisc, EntityManager::Get().GetComponent<TransformComponent>(playerId).GetPosition());
	}
}

void GameInterface::LuaPickUpMoreLaserAmmoCallback(DOG::LuaContext* context)
{
	auto& em = EntityManager::Get();
	entity player = context->GetInteger();
	assert(em.Exists(player) && em.HasComponent<BarrelComponent>(player) && em.HasComponent<LaserBarrelComponent>(player));
	auto& barrelInfo = em.GetComponent<BarrelComponent>(player);
	assert(barrelInfo.type == BarrelComponent::Type::Laser);
	auto& laserBarrel = em.GetComponent<LaserBarrelComponent>(player);
	laserBarrel.ammo += barrelInfo.ammoPerPickup;
	laserBarrel.ammo = std::min(laserBarrel.ammo, static_cast<f32>(barrelInfo.maximumAmmoCapacityForType));
	barrelInfo.currentAmmoCount = static_cast<u32>(laserBarrel.ammo);
}

void GameInterface::GetPlayerName(DOG::LuaContext* context)
{
	auto& em = EntityManager::Get();
	entity player = context->GetInteger();
	assert(em.Exists(player) && em.HasComponent<NetworkPlayerComponent>(player));
	context->ReturnString(em.GetComponent<NetworkPlayerComponent>(player).playerName);
}
