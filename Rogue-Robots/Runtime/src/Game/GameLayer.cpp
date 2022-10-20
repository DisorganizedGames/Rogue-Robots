#include "GameLayer.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	auto& am = DOG::AssetManager::Get();
	m_redCube = am.LoadModelAsset("Assets/Models/Temporary_Assets/red_cube.glb");
	m_greenCube = am.LoadModelAsset("Assets/Models/Temporary_Assets/green_cube.glb", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	m_blueCube = am.LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");
	m_magentaCube = am.LoadModelAsset("Assets/Models/Temporary_Assets/magenta_cube.glb");
	m_mixamo = am.LoadModelAsset("Assets/Models/Temporary_Assets/mixamo/walkmix.fbx");

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
	
	entity isoSphereEntity = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<ModelComponent>(isoSphereEntity, am.LoadModelAsset("Assets/Models/Temporary_Assets/iso_sphere.glb"));
	m_entityManager.AddComponent<TransformComponent>(isoSphereEntity, Vector3(20, 10, 30)).SetScale({ 2,2,2 });
	auto& isoSphereLight = m_entityManager.AddComponent<PointLightComponent>(isoSphereEntity);
	isoSphereLight.color = Vector3(0.1f, 1.0f, 0.2f);
	isoSphereLight.strength = 30;
	isoSphereLight.handle = LightManager::Get().AddPointLight(
		PointLightDesc
		{
			.position = m_entityManager.GetComponent<TransformComponent>(isoSphereEntity).GetPosition(),
			.color = isoSphereLight.color,
			.strength = isoSphereLight.strength
		},
		LightUpdateFrequency::PerFrame);
	

	//LuaMain::GetScriptManager()->OrderScript("LuaTest.lua", 1);
	//LuaMain::GetScriptManager()->OrderScript("ScriptTest.lua", -1);
	LuaMain::GetScriptManager()->SortOrderScripts();

	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");

	//Register Lua interfaces
	RegisterLuaInterfaces();
	//...
	
	/* Spawn players in a square around a given point */
	m_playerModels = {m_greenCube, m_redCube, m_blueCube, m_magentaCube};
	SpawnPlayers(Vector3(25, 25, 15), 4, 10.f);


	// Temporary door code
	entity doorTest = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<DoorComponent>(doorTest).roomId = 0;
	m_entityManager.AddComponent<TransformComponent>(doorTest, Vector3(25, 6, 15));
	m_entityManager.AddComponent<ModelComponent>(doorTest, m_magentaCube);
	
	m_entityManager.RegisterSystem(std::make_unique<DoorOpeningSystem>());

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

	// Load custom mesh and custom material (one sub-mesh + one material)
	// Material is modifiable
	{
		// Load mesh
		MeshDesc md{};
		SubmeshMetadata smMd{};

		auto mod = ShapeCreator(Shape::cone, 4).GetResult();
		md.indices = mod->mesh.indices;
		md.submeshData = mod->submeshes;
			
		for (const auto& [k, _] : mod->mesh.vertexData)
			md.vertexDataPerAttribute[k] = mod->mesh.vertexData[k];
		auto meshData = CustomMeshManager::Get().AddMesh(md);

		// Load material
		MaterialDesc d{};
		d.albedoFactor = { 1.f, 0.f, 0.f };
		d.roughnessFactor = 0.15f;
		d.metallicFactor = 0.85f;
		auto mat = CustomMaterialManager::Get().AddMaterial(d);

		auto testE = m_entityManager.CreateEntity();
		m_entityManager.AddComponent<TransformComponent>(testE, 
			DirectX::SimpleMath::Vector3{ 25.f, 10.f, 25.f }, 
			DirectX::SimpleMath::Vector3{}, 
			DirectX::SimpleMath::Vector3{ 3.f, 3.f, 3.f });
		m_entityManager.AddComponent<SubmeshRenderer>(testE, meshData.first, mat, d);	
	}
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

static f32 timeElapsed{ 0.f };

void GameLayer::OnUpdate()
{
	MINIPROFILE
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

	timeElapsed += Time::DeltaTime();

	EntityManager::Get().Collect<TransformComponent, SubmeshRenderer>().Do([&](entity e, TransformComponent& tr, SubmeshRenderer& sr)
		{
			sr.materialDesc.albedoFactor = { cosf(timeElapsed) * 0.5f + 0.5f, 0.3f * sinf(timeElapsed) * 0.5f + 0.5f, 0.2f, 1.f };
			sr.dirty = true;



		});
} 
void GameLayer::OnRender()
{
	//...
}


void GameLayer::OnImGuiRender()
{


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
	luaInterface.AddFunction<InputInterface, &InputInterface::GetMouseDelta>("MouseDelta");
	global->SetLuaInterface(luaInterface);
	//Make the object accessible from lua. Is used by: Input.FunctionName()
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Input", "InputInterface");

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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::PlayAudio>("PlayAudio");

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

						//Correct scaling for the mesh colliders (I think)
						Vector3 localMeshColliderScale = Vector3(-xFlip, yFlip, 1.0f);

						entity blockEntity = m_entityManager.CreateEntity();
						m_entityManager.AddComponent<ModelComponent>(blockEntity, aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + ".fbx"));
						TransformComponent& transform = m_entityManager.AddComponent<TransformComponent>(blockEntity,
							Vector3(x * blockDim, y * blockDim, z * blockDim),
							Vector3(piDiv2, piDiv2 * blockRot - piDiv2, 0.0f),
							Vector3(1.0f, 1.0f, 1.0f)); //Moved the scaling to later so the mesh collider is not confused by the scaling

						m_entityManager.AddComponent<ModularBlockComponent>(blockEntity);
						m_entityManager.AddComponent<MeshColliderComponent>(blockEntity,
							blockEntity, 
							aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + "_Col.fbx", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::CPUMemory | DOG::AssetLoadFlag::GPUMemory))),
							localMeshColliderScale,
							false);		// Set this to true if you want to see colliders only in wireframe

						//Sets the stupid scaling last seems to fix our problems!
						transform.SetScale(Vector3(xFlip, -1.0f * yFlip, 1.0f)); //yFlip is on Z because of left-hand/right-hand.
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

void GameLayer::SpawnPlayers(const Vector3& pos, u8 playerCount, f32 spread)
{
	ASSERT(playerCount > 0, "Need to at least spawn ThisPlayer. I.e. playerCount has to exceed 0");
	ASSERT(playerCount <= MAX_PLAYER_COUNT, "No more than 4 players can be spawned. I.e. playerCount can't exceed 4");

	auto* scriptManager = LuaMain::GetScriptManager();
	for (auto i = 0; i < playerCount; ++i)
	{
		entity playerI = m_entityManager.CreateEntity();
		Vector3 offset = {
			spread * (i % 2) - (spread / 2.f),
			0,
			spread * (i / 2) - (spread / 2.f),
		};
		m_entityManager.AddComponent<TransformComponent>(playerI, pos - offset);
		m_entityManager.AddComponent<ModelComponent>(playerI, m_playerModels[i]);
		m_entityManager.AddComponent<CapsuleColliderComponent>(playerI, playerI, 1.f, 1.8f, true, 75.f);
		auto& rb = m_entityManager.AddComponent<RigidbodyComponent>(playerI, playerI);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = false;

		m_entityManager.AddComponent<NetworkPlayerComponent>(playerI).playerId = i;
		m_entityManager.AddComponent<InputController>(playerI);
		scriptManager->AddScript(playerI, "Gun.lua");

		if (i == 0) // Only for this player
		{
			m_entityManager.AddComponent<ThisPlayer>(playerI);
			m_entityManager.AddComponent<CameraComponent>(playerI);
			m_entityManager.AddComponent<AudioListenerComponent>(playerI);
		}
		else
		{
			m_entityManager.AddComponent<OnlinePlayer>(playerI);
		}
	}
}


