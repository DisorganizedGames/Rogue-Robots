#include "GameLayer.h"

GameLayer::GameLayer() noexcept
	: Layer("Game layer")
{
	m_pipedData.viewMat = DirectX::XMMatrixLookAtLH({ 0.f, 5.f, 0.f }, { 0.f, 5.f, 1.f }, { 0.f, 1.f, 0.f });


	auto& am = DOG::AssetManager::Get();
	m_redCube = am.LoadModelAsset("Assets/red_cube.glb");
	m_greenCube = am.LoadModelAsset("Assets/green_cube.glb");
	m_blueCube = am.LoadModelAsset("Assets/blue_cube.glb");
	m_magentaCube = am.LoadModelAsset("Assets/magenta_cube.glb");

	DOG::piper::TempEntity e1;
	e1.modelID = m_redCube;
	e1.worldMatrix.Translation({ 4, -2, 5 });

	DOG::piper::TempEntity e2;
	e2.modelID = m_greenCube;
	e2.worldMatrix.Translation({ -4, -2, 5 });

	DOG::piper::TempEntity e3;
	e3.modelID = m_blueCube;
	e3.worldMatrix.Translation({ 4, 2, 5 });

	DOG::piper::TempEntity e4;
	e4.modelID = m_magentaCube;
	e4.worldMatrix.Translation({ -4, 2, 5 });

	m_pipedData.entitiesToRender.push_back(e1);
	m_pipedData.entitiesToRender.push_back(e2);
	m_pipedData.entitiesToRender.push_back(e3);
	m_pipedData.entitiesToRender.push_back(e4);

	DOG::piper::SetPipe(&m_pipedData);
}

void GameLayer::OnAttach()
{
	m_debugCam = DebugCamera(0, 1, 0);
	std::unique_ptr<SparseSet<PositionComponent>> positionComponentSet = std::make_unique<SparseSet<PositionComponent>>();

	for (uint32_t i{ 0u }; i < ENTITY_CAPACITY; i++)
	{
		positionComponentSet->sparseArray.push_back(NULL_ENTITY);
	}
	positionComponentSet->denseArray.reserve(ENTITY_CAPACITY);
	positionComponentSet->components.reserve(ENTITY_CAPACITY);
	entities.reserve(ENTITY_CAPACITY);

	mgr.components.push_back(std::move(positionComponentSet));
}

void GameLayer::OnDetach()
{

}

void GameLayer::OnUpdate()
{
	m_debugCam.OnUpdate();
	m_pipedData.viewMat = m_debugCam.GetViewMatrix();
}

void GameLayer::OnRender()
{
	//...
}

void GameLayer::AddComponent(entity entityID, const PositionComponent& component) noexcept
{
	auto positionComponentSet = static_cast<SparseSet<PositionComponent>*>(mgr.components[0].get());
	const auto pos = positionComponentSet->denseArray.size();
	positionComponentSet->denseArray.emplace_back(entityID);
	positionComponentSet->components.emplace_back(component);
	positionComponentSet->sparseArray[entityID] = (entity)pos;
}

void GameLayer::RemoveComponent(entity entityID)
{
	auto positionComponentSet = static_cast<SparseSet<PositionComponent>*>(mgr.components[0].get());
	const auto last = positionComponentSet->denseArray.back();
	std::swap(positionComponentSet->denseArray.back(), positionComponentSet->denseArray[positionComponentSet->sparseArray[entityID]]);
	std::swap(positionComponentSet->sparseArray[last], positionComponentSet->sparseArray[entityID]);
	positionComponentSet->denseArray.pop_back();
	positionComponentSet->sparseArray[entityID] = NULL_ENTITY;
}

bool GameLayer::HasComponent(entity entityID)
{
	auto positionComponentSet = static_cast<SparseSet<PositionComponent>*>(mgr.components[0].get());
	return (entityID < positionComponentSet->sparseArray.size()) && (positionComponentSet->sparseArray[entityID]< positionComponentSet->denseArray.size()) && (positionComponentSet->sparseArray[entityID] != NULL_ENTITY);
}

void GameLayer::AddEntity()
{
	std::cout << "Added entity with ID " << nextEntity << "\n";
	entities.push_back(nextEntity++);
}

void GameLayer::PrintSparseSet()
{
	auto positionComponentSet = static_cast<SparseSet<PositionComponent>*>(mgr.components[0].get());
	u32 highestEntityNr = 0u;
	for (u32 i = 0u; i < positionComponentSet->denseArray.size(); i++)
	{
		if (positionComponentSet->denseArray[i] > highestEntityNr)
			highestEntityNr = positionComponentSet->denseArray[i];
	}

	std::cout << "Sparse array: ";
	for (u32 j = 0u; j <= highestEntityNr; j++)
	{
		std::cout << "[";
		positionComponentSet->sparseArray[j] == NULL_ENTITY ? std::cout << "X" : std::cout << positionComponentSet->sparseArray[j];
		std::cout << "]";
	}
	std::cout << "\nDense array: ";
	for (u32 k = 0u; k < positionComponentSet->denseArray.size(); k++)
	{
		std::cout << "[" << positionComponentSet->denseArray[k] << "]";
	}
	std::cout << "\n\n\n";
}

void GameLayer::AddC()
{
	std::cout << "Existing entities:\n";
	for (u32 i = 0u; i < entities.size(); i++)
	{
		std::cout << entities[i] << "\n";
	}
	std::cout << "Entity to add component to: ";
	u32 chosenEntity;
	std::cin >> chosenEntity;
	std::cin.get();
	
	if (std::find(entities.begin(), entities.end(), chosenEntity) != entities.end())
	{
		AddComponent(chosenEntity, { 10.0f, 10.0f, 10.0f });
	}
}

void GameLayer::RemoveC()
{
	std::cout << "Existing entities:\n";
	for (u32 i = 0u; i < entities.size(); i++)
	{
		std::cout << entities[i] << "\n";
	}
	std::cout << "Entity to remove component for: ";
	u32 chosenEntity;
	std::cin >> chosenEntity;
	std::cin.get();
	RemoveComponent(chosenEntity);
}

void GameLayer::HasC()
{
	std::cout << "Existing entities:\n";
	for (u32 i = 0u; i < entities.size(); i++)
	{
		std::cout << entities[i] << "\n";
	}
	std::cout << "Entity to check for component-owning : ";
	u32 chosenEntity;
	std::cin >> chosenEntity;
	std::cin.get();
	std::cout << HasComponent(chosenEntity) << "\n";
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
		Key k = EVENT(KeyPressedEvent).key;
		if (k == Key::A)
			AddEntity();
		else if (k == Key::C)
			AddC();
		else if (k == Key::P)
			PrintSparseSet();
		else if (k == Key::R)
			RemoveC();
		else if (k == Key::H)
			HasC();
		break;
	}
	}
}
