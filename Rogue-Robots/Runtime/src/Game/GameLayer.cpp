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

	// Temporary solution to not have the entity manager crash on audio system
	entity testAudio = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<AudioComponent>(testAudio);

	entity entity2 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity2, m_greenCube);
	m_entityManager.AddComponent<TransformComponent>(entity2, Vector3(-4, -2, 5), Vector3(0.1f, 0, 0));
	

	entity entity3 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity3, m_blueCube);
	auto& t3 = m_entityManager.AddComponent<TransformComponent>(entity3);
	
	t3.SetPosition({ 4, 2, 5 });
	t3.SetScale({ 0.5f, 0.5f, 0.5f });

	entity entity4 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity4, m_magentaCube);
	auto& t4 = m_entityManager.AddComponent<TransformComponent>(entity4);
	m_entityManager.AddComponent<NetworkPlayerComponent>(entity4).playerId = 3;
	t4.worldMatrix(3, 0) = 39;
	t4.worldMatrix(3, 1) = 30;
	t4.worldMatrix(3, 2) = 40;

	entity entity5 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity5, m_mixamo);
	m_entityManager.AddComponent<TransformComponent>(entity5, Vector3(0, -2, 5), Vector3(0, 0, 0), Vector3(0.02f, 0.02f, 0.02f));
	m_entityManager.AddComponent<AnimationComponent>(entity5).offset = 0;

	m_entityManager.AddComponent<SphereColliderComponent>(entity3, entity3, 1.0f, true);
	m_entityManager.AddComponent<CapsuleColliderComponent>(entity5, entity5, 1.0f, 1.0f, false);
	m_entityManager.AddComponent<BoxColliderComponent>(entity4, entity4, Vector3(1, 1, 1), true);
	m_entityManager.AddComponent<MeshColliderComponent>(entity2, entity2, m_greenCube);
	m_entityManager.AddComponent<RigidbodyComponent>(entity4, entity4);
	m_entityManager.AddComponent<RigidbodyComponent>(entity3, entity3);
	//rigidbodyComponent.SetOnCollisionEnter([&](entity ent1, entity ent2) {std::cout << ent1 << " " << ent2 << " Enter Hello\n"; });
	//rigidbodyComponent.SetOnCollisionExit([&](entity ent1, entity ent2) {std::cout << ent1 << " " << ent2 << " Exit Hello\n"; });

	/*m_entityManager.GetComponent<RigidbodyComponent>(entity4).ConstrainPosition(true, false, true);*/
	//m_entityManager.GetComponent<RigidbodyComponent>(entity4).ConstrainRotation(false, true, true);

	entity entity80 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity80, m_blueCube);
	auto& t80 = m_entityManager.AddComponent<TransformComponent>(entity80);
	t80.worldMatrix = t3.worldMatrix;
	m_entityManager.AddComponent<SphereColliderComponent>(entity80, entity80, 1.0f, true);
	m_entityManager.AddComponent<RigidbodyComponent>(entity80, entity80);

	entity entity81 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(entity81, m_magentaCube);
	auto& t81 = m_entityManager.AddComponent<TransformComponent>(entity81);
	t81.worldMatrix = t4.worldMatrix;
	m_entityManager.AddComponent<BoxColliderComponent>(entity81, entity81, Vector3(1, 1, 1), true);
	m_entityManager.AddComponent<RigidbodyComponent>(entity81, entity81);
	


	LuaMain::Initialize();
	//LuaMain::GetScriptManager()->OrderScript("LuaTest.lua", 1);
	//LuaMain::GetScriptManager()->OrderScript("ScriptTest.lua", -1);
	LuaMain::GetScriptManager()->SortOrderScripts();

	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");


	//Register Lua interfaces
	RegisterLuaInterfaces();
	//...

	entity Player1 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(Player1, m_greenCube);
	m_entityManager.AddComponent<TransformComponent>(Player1, Vector3(-10, 0, 0), Vector3(0.1f, 0, 0), Vector3(0.5f, 0.5f, 0.5f));
	m_entityManager.AddComponent<NetworkPlayerComponent>(Player1).playerId = 0;
	m_entityManager.AddComponent<InputController>(Player1);
	m_entityManager.AddComponent<CameraComponent>(Player1);
	m_entityManager.AddComponent<ThisPlayer>(Player1);
	ScriptManager* scriptManager = LuaMain::GetScriptManager();
	scriptManager->AddScript(Player1, "Gun.lua");

	entity Player2 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(Player2, m_redCube);
	m_entityManager.AddComponent<TransformComponent>(Player2, Vector3(-10, 10, 0), Vector3(0.1f, 0, 0), Vector3(0.5f, 0.5f, 0.5f));
	m_entityManager.AddComponent<NetworkPlayerComponent>(Player2).playerId = 1;
	m_entityManager.AddComponent<InputController>(Player2);
	m_entityManager.AddComponent<OnlinePlayer>(Player2);
	scriptManager->AddScript(Player2, "Gun.lua");

	entity Player3 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(Player3, m_blueCube);
	m_entityManager.AddComponent<TransformComponent>(Player3, Vector3(-10, 20, 0), Vector3(0.1f, 0, 0), Vector3(0.5f, 0.5f, 0.5f));
	m_entityManager.AddComponent<NetworkPlayerComponent>(Player3).playerId = 2;
	m_entityManager.AddComponent<InputController>(Player3);
	m_entityManager.AddComponent<OnlinePlayer>(Player3);
	scriptManager->AddScript(Player3, "Gun.lua");
	
	entity Player4 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(Player4, m_magentaCube);
	m_entityManager.AddComponent<TransformComponent>(Player4, Vector3(-10, 30, 0), Vector3(0.1f, 0, 0), Vector3(0.5f, 0.5f, 0.5f));
	m_entityManager.AddComponent<NetworkPlayerComponent>(Player4).playerId = 3;
	m_entityManager.AddComponent<InputController>(Player4);
	m_entityManager.AddComponent<OnlinePlayer>(Player4);
	scriptManager->AddScript(Player4, "Gun.lua");

	// Setup lights

	// Default lights
	u32 xOffset = 18;
	u32 zOffset = 18;
	for (u32 i = 0; i < 3; ++i)
	{
		for (u32 x = 0; x < 3; ++x)
		{
			auto pdesc = PointLightDesc();
			pdesc.position = { xOffset + (f32)i * 7.f, 8.f, zOffset + (f32)x * 7.f };
			pdesc.color = { 1.f, 0.f, 0.f };
			pdesc.strength = 10.f;
			LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::Never);

			auto dd = SpotLightDesc();
			dd.position = { xOffset + (f32)i * 7.f, 16.f, zOffset + (f32)x * 7.f };
			dd.color = { 0.f, 0.f, 1.f };
			dd.direction = { 0.f, 1.f, 0.f };
			dd.strength = 1.f;
			LightManager::Get().AddSpotLight(dd, LightUpdateFrequency::Never);
		}
	}

	// Moving light
	LightHandle pointLight = LightManager::Get().AddPointLight(PointLightDesc(), LightUpdateFrequency::PerFrame);
	m_movingPointLight = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<TransformComponent>(m_movingPointLight, Vector3(10, 10, 10), Vector3(0, 0, 0), Vector3(1.f));
	m_entityManager.AddComponent<PointLightComponent>(m_movingPointLight, pointLight, Vector3(1.f, 1.f, 0.f), 5.f);
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
	for (auto& system : m_entityManager)
	{
		system->EarlyUpdate();
	}
	for (auto& system : m_entityManager)
	{
		system->Update();
	}
	for (auto& system : m_entityManager)
	{
		system->LateUpdate();
	}
	
	m_elapsedTime += Time::DeltaTime();
	m_entityManager.Get().GetComponent<PointLightComponent>(m_movingPointLight).dirty = true;
	m_entityManager.Get().GetComponent<TransformComponent>(m_movingPointLight).SetPosition(Vector3(
		15.f, 
		8.f, 
		10.f + 30.f * (cosf((f32)m_elapsedTime) * 0.5f + 0.5f)));


	LuaGlobal* global = LuaMain::GetGlobal();
	global->SetNumber("DeltaTime", Time::DeltaTime());

	m_player->OnUpdate();
	m_netCode.OnUpdate();
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
		EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer& )
			{
				inputC.shoot = true;
			});
		break;
	}
	case EventType::LeftMouseButtonReleasedEvent:
	{
		EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
			{
				inputC.shoot = false;
			});
		break;
	}
	case EventType::KeyPressedEvent:
	{
		if (EVENT(KeyPressedEvent).key == DOG::Key::C)
			m_player->m_moveView = !m_player->m_moveView;
		else
			Input(EVENT(KeyPressedEvent).key);
		break;
	}
	case EventType::KeyReleasedEvent:
	{
		Release(EVENT(KeyReleasedEvent).key);
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetUp>("GetUp");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetForward>("GetForward");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetRight>("GetRight");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAction>("GetAction");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetAction>("SetAction");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::HasComponent>("HasComponent");

	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Entity", "EntityInterface");

	//-----------------------------------------------------------------------------------------------
	//Assets
	luaInterfaceObject = std::make_shared<AssetInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("AssetInterface");
	luaInterface.AddFunction<AssetInterface, &AssetInterface::LoadModel>("LoadModel");
	luaInterface.AddFunction<AssetInterface, &AssetInterface::LoadAudio>("LoadAudio");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Asset", "AssetInterface");

	//-----------------------------------------------------------------------------------------------
	//Host
	m_player = std::make_shared<MainPlayer>();
	luaInterfaceObject = std::make_shared<HostInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("HostInterface");
	luaInterface.AddFunction<HostInterface, &HostInterface::DistanceToPlayers>("DistanceToPlayers");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Host", "HostInterface");

	//-----------------------------------------------------------------------------------------------
	//Physics
	luaInterfaceObject = std::make_shared<PhysicsInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("PhysicsInterface");
	luaInterface.AddFunction<PhysicsInterface, &PhysicsInterface::RBSetVelocity>("RBSetVelocity");
	
	global->SetLuaInterface(luaInterface);
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Physics", "PhysicsInterface");
}

void GameLayer::LoadLevel()
{
	float blockDim = 5.0f;

	std::string line;
	std::ifstream inputFile("..\\Offline-Tools\\PCG\\showOff_generatedLevel.txt");

	AssetManager& aManager = AssetManager::Get();

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
					if (block != "Empty" && block != "Void" && block != "q")
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

						entity blockEntity = m_entityManager.CreateEntity();
						m_entityManager.AddComponent<ModelComponent>(blockEntity, aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + ".fbx"));
						m_entityManager.AddComponent<TransformComponent>(blockEntity)
							.SetPosition({ x * blockDim, y * blockDim, z * blockDim })
							.SetRotation({ piDiv2, piDiv2 * blockRot - piDiv2, 0.0f })
							.SetScale({ xFlip, -1.0f * yFlip, 1.0f }); //yFlip is on Z because of left-hand/right-hand.
						m_entityManager.AddComponent<ModularBlockComponent>(blockEntity);
						m_entityManager.AddComponent<MeshColliderComponent>(blockEntity,
							blockEntity, 
							aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + "_Col.fbx", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::CPUMemory | DOG::AssetLoadFlag::GPUMemory))),
							false);		// Set this to true if you want to see colliders only in wireframe
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

void GameLayer::Input(DOG::Key key)
{
	EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
	{
			if (key == DOG::Key::W)
				inputC.forward = true;
			if (key == DOG::Key::A)
				inputC.left = true;
			if (key == DOG::Key::S)
				inputC.backwards = true;
			if (key == DOG::Key::D)
				inputC.right = true;
			if (key == DOG::Key::LShift)
				inputC.down = true;
			if (key == DOG::Key::Spacebar)
				inputC.up = true;
			if (key == DOG::Key::Q)
				inputC.switchComp = true;

	});
}

void GameLayer::Release(DOG::Key key)
{
	EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
		{
			if (key == DOG::Key::W)
				inputC.forward = false;
			if (key == DOG::Key::A)
				inputC.left = false;
			if (key == DOG::Key::S)
				inputC.backwards = false;
			if (key == DOG::Key::D)
				inputC.right = false;
			if (key == DOG::Key::LShift)
				inputC.down = false;
			if (key == DOG::Key::Spacebar)
				inputC.up = false;
			if (key == DOG::Key::Q)
				inputC.switchComp = false;
		});
}


