#include "LuaInterfaces.h"
#include "GameComponent.h"

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

//---------------------------------------------------------------------------------------------------------
//Audio
void AudioInterface::Play(LuaContext* context)
{
	auto path = context->GetString();
	auto audioAsset = AssetManager::Get().LoadAudio(path, AssetLoadFlag::CPUMemory);

	static auto tempEntityForLuaAudio = EntityManager::Get().CreateEntity();
	auto& audioComponent = EntityManager::Get().AddComponent<AudioComponent>(tempEntityForLuaAudio);

	audioComponent.assetID = audioAsset;
	audioComponent.shouldPlay = true;
}

//---------------------------------------------------------------------------------------------------------
//Entity
void EntityInterface::CreateEntity(LuaContext* context)
{
	context->ReturnInteger(EntityManager::Get().CreateEntity());
}

void EntityInterface::DestroyEntity(LuaContext* context)
{
	EntityManager::Get().DestroyEntity(context->GetInteger());
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
	else if (compType == "Network")
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
	else if (compType == "Rigidbody")
	{
		AddRigidbody(context, e);
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
//1. Shoot, 2. Jump, 3.activateActiveItem 
void EntityInterface::GetAction(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	int action = context->GetInteger();
	InputController& input = EntityManager::Get().GetComponent<InputController>(e);
	switch (action)
	{
	case 1:
		if (input.shoot)
			context->ReturnBoolean(true);
		else
			context->ReturnBoolean(false);
		break;
	case 2:
		if (input.jump)
			context->ReturnBoolean(true);
		else
			context->ReturnBoolean(false);
		break;
	case 3:
		if (input.activateActiveItem)
			context->ReturnBoolean(true);
		else
			context->ReturnBoolean(false);
		break;
	case 4:
		if (input.normalFireMode)
			context->ReturnBoolean(true);
		else
			context->ReturnBoolean(false);
		break;
	default:
		break;
	}
	
}void EntityInterface::SetAction(DOG::LuaContext* context)
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
		input.normalFireMode = active;
		break;
	default:
		break;
	}

}

#pragma region HasComponent

template<typename ComponentType>
bool HasComp(entity e)
{
	return EntityManager::Get().HasComponent<ComponentType>(e);
}

const std::unordered_map<std::string, bool (*) (entity)> componentMap = {
	{ "Transform", HasComp<TransformComponent> },
	{ "Model", HasComp<ModelComponent> },
	{ "Audio", HasComp<AudioComponent>},
	{ "Network", HasComp<NetworkComponent>},
	{ "Rigidbody", HasComp<RigidbodyComponent>},
	{ "BoxCollider", HasComp<BoxColliderComponent>},
};

void EntityInterface::HasComponent(LuaContext* context)
{
	entity e = context->GetInteger();
	bool hasComp = componentMap.at(context->GetString())(e);
	context->ReturnBoolean(hasComp);
}

#pragma endregion

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

void EntityInterface::GetPlayerStats(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	PlayerStatsComponent& psComp = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);

	LuaTable t;
	t.AddDoubleToTable("health", psComp.health);
	t.AddDoubleToTable("maxHealth", psComp.maxHealth);
	t.AddDoubleToTable("speed", psComp.speed);

	context->ReturnTable(t);
}

void EntityInterface::AddModel(LuaContext* context, entity e)
{
	EntityManager::Get().AddComponent<ModelComponent>(e, static_cast<u32>(std::stoull(context->GetString())));
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
	EntityManager::Get().AddComponent<NetworkComponent>(e);
}

void EntityInterface::AddAgentStats(LuaContext* context, entity e)
{
	LuaTable stats = context->GetTable();

	auto& agentStats = EntityManager::Get().AddComponent<AgentStatsComponent>(e);
	agentStats = {
		.hp = stats.GetFloatFromTable("hp"),
		.maxHP = stats.GetFloatFromTable("maxHP"),
		.speed = stats.GetFloatFromTable("speed")
	};
}

void EntityInterface::AddAudio(LuaContext* context, entity e)
{
	auto assetID = context->GetInteger();
	auto shouldPlay = context->GetBoolean();

	auto& comp = EntityManager::Get().AddComponent<AudioComponent>(e);

	comp.assetID = assetID;
	comp.shouldPlay = shouldPlay;
}

void EntityInterface::AddBoxCollider(LuaContext* context, entity e)
{
	LuaTable boxDimTable = context->GetTable();
	bool dynamic = context->GetBoolean();

	LuaVector3 boxDim = LuaVector3(boxDimTable);

	EntityManager::Get().AddComponent<BoxColliderComponent>(e, e, Vector3{ boxDim.x, boxDim.y, boxDim.z }, dynamic);
}

void EntityInterface::AddRigidbody(LuaContext* context, entity e)
{
	bool kinematic = context->GetBoolean();
	EntityManager::Get().AddComponent<RigidbodyComponent>(e, e, kinematic);
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

//---------------------------------------------------------------------------------------------------------
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
		[&](entity id, ThisPlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
			distances.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
		});

	EntityManager::Get().Collect<OnlinePlayer, TransformComponent, NetworkPlayerComponent>().Do(
		[&](entity id, OnlinePlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
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
