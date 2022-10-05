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
	//Add more component types here.
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
	Vector3 forward = Vector3::Transform({ 0, 0, 1 }, transform.GetRotation());
	
	LuaTable t;
	t.AddFloatToTable("x", forward.x);
	t.AddFloatToTable("y", forward.y);
	t.AddFloatToTable("z", forward.z);
	context->ReturnTable(t);
}

void EntityInterface::GetUp(LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	Vector3 up = Vector3::Transform({ 0, 1, 0 }, transform.GetRotation());

	LuaTable t;
	t.AddFloatToTable("x", up.x);
	t.AddFloatToTable("y", up.y);
	t.AddFloatToTable("z", up.z);
	context->ReturnTable(t);
}

void EntityInterface::GetRight(DOG::LuaContext* context)
{
	entity e = context->GetInteger();
	TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
	Vector3 right;
	right = Vector3::Transform({ 1, 0, 0 }, transform.GetRotation());

	LuaTable t;
	t.AddFloatToTable("x", right.x);
	t.AddFloatToTable("y", right.y);
	t.AddFloatToTable("z", right.z);
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
	default:
		break;
	}
	
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
//Player
void PlayerInterface::GetID(LuaContext* context)
{
	context->ReturnInteger(m_player);
}

LuaVector3::LuaVector3(LuaTable& table)
{
	x = table.GetFloatFromTable("x");
	y = table.GetFloatFromTable("y");
	z = table.GetFloatFromTable("z");
}

