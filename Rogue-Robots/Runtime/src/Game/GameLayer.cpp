#include "GameLayer.h"

GameLayer::GameLayer() noexcept
	: Layer("Game layer")
{
	m_pipedData.viewMat = DirectX::XMMatrixLookAtLH({ 0.f, 5.f, 0.f }, { 0.f, 5.f, 1.f }, { 0.f, 1.f, 0.f });

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
	//m_debugCam.PrintPosition();
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
		auto [x, y] = EVENT(LeftMouseButtonPressedEvent).coordinates;
		std::cout << GetName() << " received event: Left MB clicked [x,y] = [" << x << "," << y << "]\n";
		break;
	}
	}
}
