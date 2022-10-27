#include "LuaInterfaces.h"
#include "GameComponent.h"
#include "ExplosionSystems.h"

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
	else if (compType == "SubMeshRender")
	{
		AddSubmeshRender(context, e);
	}
	//Add more component types here.
	else
	{
		assert(false && "Lua can't create component");
	}
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


void EntityInterface::AgentHit(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	entity bullet = context->GetInteger();
	if (!EntityManager::Get().HasComponent<AgentHitComponent>(e))
		EntityManager::Get().AddComponent<AgentHitComponent>(e).HitBy(bullet);
	else
		EntityManager::Get().GetComponent<AgentHitComponent>(e).HitBy(bullet);
}

void EntityInterface::IsBulletLocal(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	BulletComponent& bullet = EntityManager::Get().GetComponent<BulletComponent>(e);
	EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer>().Do([&](NetworkPlayerComponent& networkC, ThisPlayer&)
		{
			if (networkC.playerId == bullet.playerId)
			{
				context->ReturnBoolean(true);
			}
			else
			{
				context->ReturnBoolean(false);
			}
		});

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

const std::unordered_map<PassiveItemComponent::Type, std::string> passiveTypeMap = {
	{ PassiveItemComponent::Type::Template, "Template" },
	{ PassiveItemComponent::Type::MaxHealthBoost, "MaxHealthBoost" },
	{ PassiveItemComponent::Type::SpeedBoost, "SpeedBoost" },
	{ PassiveItemComponent::Type::LifeSteal, "LifeSteal" },
};

void EntityInterface::GetPassiveType(LuaContext* context)
{
	entity e = context->GetInteger();
	auto type = EntityManager::Get().GetComponent<PassiveItemComponent>(e).type;
	context->ReturnString(passiveTypeMap.at(type));
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

	Matrix rotMat = DirectX::XMMatrixLookToLH((DirectX::XMVECTOR)transform.GetPosition(), forward, up);
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
	NetworkPlayerComponent& player = EntityManager::Get().GetComponent<NetworkPlayerComponent>(playerEntity);
	EntityManager::Get().AddComponent<BulletComponent>(e).playerId =(i8)player.playerId;
	if (EntityManager::Get().HasComponent<RigidbodyComponent>(e))
		EntityManager::Get().GetComponent<RigidbodyComponent>(e).continuousCollisionDetection = true;
}

void EntityInterface::AddSubmeshRender(LuaContext* context, entity e)
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
}

void EntityInterface::AddScript(DOG::LuaContext* context, DOG::entity e)
{
	LuaMain::GetScriptManager()->AddScript(e, context->GetString());
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

	LuaVector3 albedoFactor(table);

	MaterialDesc d{};
	d.albedoFactor = { albedoFactor.x, albedoFactor.y, albedoFactor.z };
	d.roughnessFactor = roughnessFactor;
	d.metallicFactor = metallicFactor;
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

	context->ReturnTable(material);
}

void GameInterface::ExplosionEffect(DOG::LuaContext* context)
{
	context->ReturnInteger(ExplosionEffectSystem::CreateExplosionEffect(context->GetInteger(), (float)context->GetDouble()));
}
