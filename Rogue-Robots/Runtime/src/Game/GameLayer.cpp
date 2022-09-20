#include "GameLayer.h"

GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }
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
