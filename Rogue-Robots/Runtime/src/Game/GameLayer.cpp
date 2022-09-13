#include "GameLayer.h"

GameLayer::GameLayer() noexcept
	: Layer("Game layer")
{
}

void GameLayer::OnAttach()
{
	//...
}

void GameLayer::OnDetach()
{
	//...
}

void GameLayer::OnUpdate()
{
	//...
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
