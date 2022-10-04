#include "EmilFDebugLayer.h"


EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil debug layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	
}


void EmilFDebugLayer::OnAttach()
{
	m_systems.emplace_back(std::make_unique<DOG::TestSystem>());
	m_systems[0]->Create();
	m_systems[0]->Update();
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
