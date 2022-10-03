#include "GameLayer.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;
GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	auto& am = DOG::AssetManager::Get();
	m_redCube = am.LoadModelAsset("Assets/red_cube.glb");
	m_greenCube = am.LoadModelAsset("Assets/green_cube.glb", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	m_blueCube = am.LoadModelAsset("Assets/blue_cube.glb");
	m_magentaCube = am.LoadModelAsset("Assets/magenta_cube.glb");
	m_mixamo = am.LoadModelAsset("Assets/mixamo/walkmix.fbx");

	// Create some shapes
	{
		u32 tessFactor[3] = { 1, 10, 100 };
		for (u32 i = 0; i < 3; i++) // 3 sheets
			m_shapes.push_back(am.LoadShapeAsset(Shape::sheet, tessFactor[i]));
		for (u32 i = 0; i < 3; i++) // 3 spheres
			m_shapes.push_back(am.LoadShapeAsset(Shape::sphere, 2 + tessFactor[i], 2 + tessFactor[i]));
		for (u32 i = 0; i < 3; i++) // 3 cones
			m_shapes.push_back(am.LoadShapeAsset(Shape::cone, 2 + tessFactor[i], 2 + tessFactor[i]));
		for (u32 i = 0; i < 3; i++) // 3 prisms
			m_shapes.push_back(am.LoadShapeAsset(Shape::prism, 2 + tessFactor[i], 2 + tessFactor[i]));
		
		for (i32 i = 0; i < 4; i++)
		{
			for (i32 j = 0; j < 3; j++)
			{
				entity e = m_entityManager.CreateEntity();
				m_entityManager.AddComponent<ModelComponent>(e, m_shapes[i*3+j]);
				m_entityManager.AddComponent<TransformComponent>(e, Vector3(f32(-3 + j * 3), (f32)(-4.5f + i * 3), 10), Vector3(0, 0, 0));
			}
		}
		entity xAxis = m_entityManager.CreateEntity();
		m_entityManager.AddComponent<ModelComponent>(xAxis, m_shapes[9]);
		m_entityManager.AddComponent<TransformComponent>(xAxis, Vector3(0, 0, 0), Vector3(0, 0, DirectX::XM_PIDIV2), Vector3(0.02f, 100, 0.02f));
		entity yAxis = m_entityManager.CreateEntity();
		m_entityManager.AddComponent<ModelComponent>(yAxis, m_shapes[10]);
		m_entityManager.AddComponent<TransformComponent>(yAxis, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0.02f, 100, 0.02f));
		entity zAxis = m_entityManager.CreateEntity();
		m_entityManager.AddComponent<ModelComponent>(zAxis, m_shapes[11]);
		m_entityManager.AddComponent<TransformComponent>(zAxis, Vector3(0, 0, 0), Vector3(DirectX::XM_PIDIV2, 0, 0), Vector3(0.02f, 100, 0.02f));
	}

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
	m_entityManager.AddComponent<AnimationComponent>(entity5).offset = 0;

	m_entityManager.AddComponent<SphereColliderComponent>(entity3, entity3, 1.0f, true);
	m_entityManager.AddComponent<CapsuleColliderComponent>(entity4, entity4, 1.0f, 1.0f, true);
	m_entityManager.AddComponent<BoxColliderComponent>(entity5, entity5, Vector3(1, 1, 1), false);
	m_entityManager.AddComponent<MeshColliderComponent>(entity2, entity2, m_greenCube);
	m_entityManager.AddComponent<RigidbodyComponent>(entity4, entity4);
	/*m_entityManager.GetComponent<RigidbodyComponent>(entity4).ConstrainPosition(true, false, true);*/
	//m_entityManager.GetComponent<RigidbodyComponent>(entity4).ConstrainRotation(false, true, true);

	LuaMain::Initialize();
	//LuaMain::GetScriptManager()->OrderScript("LuaTest.lua", 1);
	//LuaMain::GetScriptManager()->OrderScript("ScriptTest.lua", -1);
	LuaMain::GetScriptManager()->SortOrderScripts();

	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");


	//Register Lua interfaces
	RegisterLuaInterfaces();
	//...
}


void GameLayer::OnAttach()
{
	LoadLevel();
	m_Agent = std::make_shared<Agent>();
	LuaMain::GetScriptManager()->StartScripts();
}

void GameLayer::OnDetach()
{

}

void GameLayer::OnUpdate()
{
	LuaGlobal* global = LuaMain::GetGlobal();
	global->SetNumber("DeltaTime", Time::DeltaTime());
	
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
	case EventType::KeyPressedEvent:
	{
		if (EVENT(KeyReleasedEvent).key == DOG::Key::C)
			m_player->m_moveView = !m_player->m_moveView;
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
	luaInterface.AddFunction<AudioInterface, &AudioInterface::Play>("Play");
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetRotationForwardUp>("SetRotationForwardUp");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetPlayerStats>("GetPlayerStats");
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
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetID>("GetID");
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetForward>("GetForward");
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetUp>("GetUp");
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetRight>("GetRight");
	luaInterface.AddFunction<PlayerInterface, &PlayerInterface::GetPosition>("GetPosition");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Player", "PlayerInterface");
}

void GameLayer::LoadLevel()
{
	float blockDim = 5.0f;

	std::string line;
	std::ifstream inputFile("..\\Offline-Tools\\PCG\\testLevelOutput_generatedLevel.txt");

	AssetManager& aManager = AssetManager::Get();
	EntityManager& eManager = EntityManager::Get();

	unsigned x = 0;
	unsigned y = 0;
	unsigned z = 0;
	float piDiv2 = DirectX::XM_PIDIV2;
	if (inputFile.is_open())
	{
		while (std::getline(inputFile, line))
		{
			if (line[0] != '-')
			{
				while (line.find(' ') != std::string::npos)
				{
					size_t delimPos = line.find(' ');
					std::string block = line.substr(0, delimPos);
					line.erase(0, delimPos + 1);
					if (block != "Empty")
					{
						size_t firstUnderscore = block.find('_');
						size_t secondUnderscore = block.find('_', firstUnderscore + 1);
						std::string blockName = block.substr(0, firstUnderscore);
						int blockRot = std::stoi(block.substr(firstUnderscore + 2, secondUnderscore - firstUnderscore - 2));
						std::string blockFlip = block.substr(secondUnderscore + 1, block.size() - secondUnderscore - 1);

						float xFlip = 1.0f;
						float yFlip = 1.0f;
						if (blockFlip.find('x') != std::string::npos)
						{
							xFlip = -1.0f;
						}
						if (blockFlip.find('y') != std::string::npos)
						{
							yFlip = -1.0f;
						}

						entity blockEntity = eManager.CreateEntity();
						eManager.AddComponent<ModelComponent>(blockEntity, aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + ".fbx"));
						eManager.AddComponent<TransformComponent>(blockEntity)
							.SetPosition({ x * blockDim, y * blockDim, z * blockDim })
							.SetRotation({ piDiv2, piDiv2 * blockRot - piDiv2, 0.0f })
							.SetScale({ xFlip, -1.0f, yFlip }); //yFlip is on Z because of left-hand/right-hand.
						eManager.AddComponent<ModularBlockComponent>(blockEntity);
					}

					++x;
				}
				x = 0;
				++y;
			}
			else
			{
				x = 0;
				y = 0;
				++z;
			}
		}
	}
}