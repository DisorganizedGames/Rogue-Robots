#include "GameLayer.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;
GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	m_boneJourno = std::make_unique<AnimationManager>();
	auto& am = DOG::AssetManager::Get();
	m_redCube = am.LoadModelAsset("Assets/red_cube.glb");
	m_greenCube = am.LoadModelAsset("Assets/green_cube.glb");
	m_blueCube = am.LoadModelAsset("Assets/blue_cube.glb");
	m_magentaCube = am.LoadModelAsset("Assets/magenta_cube.glb");
	m_mixamo = am.LoadModelAsset("Assets/mixamo/walkmix.fbx");

	entity entity2 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity2, m_greenCube);
	m_entityManager.AddComponent<TransformComponent>(entity2, Vector3(-4, -2, 5), Vector3(0.1f, 0, 0));
	m_entityManager.AddComponent<NetworkPlayerComponent>(entity2).playerId = 1;

	entity entity3 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity3, m_blueCube);
	auto& t3 = m_entityManager.AddComponent<TransformComponent>(entity3);
	m_entityManager.AddComponent<NetworkPlayerComponent>(entity3).playerId = 2;
	t3.SetPosition({ 4, 2, 5 });
	t3.SetScale({ 0.5f, 0.5f, 0.5f });

	entity entity4 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity4, m_magentaCube);
	auto& t4 = m_entityManager.AddComponent<TransformComponent>(entity4);
	m_entityManager.AddComponent<NetworkPlayerComponent>(entity4).playerId = 3;
	t4.worldMatrix(3, 0) = -4;
	t4.worldMatrix(3, 1) = 2;
	t4.worldMatrix(3, 2) = 5;

	entity entity5 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity5, m_mixamo);
	m_entityManager.AddComponent<TransformComponent>(entity5, Vector3(0, -2, 5), Vector3(0, 0, 0), Vector3(0.02f, 0.02f, 0.02f));
	m_entityManager.AddComponent<AnimationComponent>(entity5).animationID = 0;

	LuaMain::Initialize();
	//LuaMain::GetScriptManager()->OrderScript("LuaTest.lua", 1);
	//LuaMain::GetScriptManager()->OrderScript("ScriptTest.lua", -1);
	LuaMain::GetScriptManager()->SortOrderScripts();
}

void GameLayer::OnAttach()
{
	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");

	//Register Lua interfaces
	RegisterLuaInterfaces();
	//...

	LuaMain::GetScriptManager()->StartScripts();
}

void GameLayer::OnDetach()
{

}

void GameLayer::OnUpdate()
{
	LuaGlobal* global = LuaMain::GetGlobal();
	global->SetNumber("DeltaTime", Time::DeltaTime());
	
	//m_boneJourno->UpdateSkeleton(0, 1, 0.1f);
	m_player->OnUpdate();
	m_netCode.OnUpdate(m_player);

	LuaMain::GetScriptManager()->UpdateScripts();
	LuaMain::GetScriptManager()->ReloadScripts();
}



void GameLayer::OnRender()
{
	//...
}

void GameLayer::OnImGuiRender()
{
	//...
}

//Place-holder example on how to use event system:
void GameLayer::OnEvent(DOG::IEvent& event)
{
	using namespace DOG;
	switch (event.GetEventType())
	{
	case EventType::LeftMouseButtonPressedEvent:
	{
		//auto [x, y] = EVENT(LeftMouseButtonPressedEvent).coordinates;
		//std::cout << GetName() << " received event: Left MB clicked [x,y] = [" << x << "," << y << "]\n";
		break;
	}
	}
}

void GameLayer::RegisterLuaInterfaces()
{
	LuaGlobal* global = LuaMain::GetGlobal();

	global->SetNumber("DeltaTime", Time::DeltaTime());

	//-----------------------------------------------------------------------------------------------
	//Input
	//Create a luaInterface variable that holds the interface object (is reused for all interfaces)
	std::shared_ptr<LuaInterface> luaInterfaceObject = std::make_shared<InputInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject); //Add it to the gamelayer's interfaces.
	
	auto luaInterface = global->CreateLuaInterface("InputInterface"); //Register a new interface in lua.
	//Add all functions that are needed from the interface class.
	luaInterface.AddFunction<InputInterface, &InputInterface::IsLeftPressed>("IsLeftPressed");
	luaInterface.AddFunction<InputInterface, &InputInterface::IsRightPressed>("IsRightPressed");
	luaInterface.AddFunction<InputInterface, &InputInterface::IsKeyPressed>("IsKeyPressed");
	global->SetLuaInterface(luaInterface);
	//Make the object accessible from lua. Is used by: Input.FunctionName()
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Input", "InputInterface");

	//-----------------------------------------------------------------------------------------------
	//Audio
	luaInterfaceObject = std::make_shared<AudioInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("AudioInterface");
	//luaInterface.AddFunction<AudioInterface, &InputInterface::PlaySound>("PlaySound");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Audio", "AudioInterface");

	//-----------------------------------------------------------------------------------------------
	//Entities
	luaInterfaceObject = std::make_shared<EntityInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("EntityInterface");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::CreateEntity>("CreateEntity");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::DestroyEntity>("DestroyEntity");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::AddComponent>("AddComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::ModifyComponent>("ModifyComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetTransformPosData>("GetTransformPosData");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Entity", "EntityInterface");

	//-----------------------------------------------------------------------------------------------
	//Assets
	luaInterfaceObject = std::make_shared<AssetInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("AssetInterface");
	luaInterface.AddFunction<AssetInterface, &AssetInterface::LoadModel>("LoadModel");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Asset", "AssetInterface");

	//-----------------------------------------------------------------------------------------------
	//Player
	m_player = std::make_shared<MainPlayer>();
	luaInterfaceObject = std::make_shared<PlayerInterface>(m_player->GetEntity());
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("PlayerInterface");
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetForward>("GetForward");
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetPosition>("GetPosition");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Player", "PlayerInterface");
}