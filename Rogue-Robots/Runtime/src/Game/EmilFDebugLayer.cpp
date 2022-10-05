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
