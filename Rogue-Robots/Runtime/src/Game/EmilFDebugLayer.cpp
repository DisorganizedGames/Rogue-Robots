#include "EmilFDebugLayer.h"

EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil debug layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	
}

void EmilFDebugLayer::OnAttach()
{
	
}

void EmilFDebugLayer::OnDetach()
{

}

void EmilFDebugLayer::OnUpdate()
{

}

void EmilFDebugLayer::OnRender()
{
	//...
}

void EmilFDebugLayer::OnImGuiRender()
{
	//...
}

void EmilFDebugLayer::OnEvent(DOG::IEvent&)
{
	
}
