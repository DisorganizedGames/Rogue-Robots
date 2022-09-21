#include "EmilDebugLayer.h"

EmilDebugLayer::EmilDebugLayer() noexcept
	: Layer("Emil debug layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	
}

void EmilDebugLayer::OnAttach()
{
	using namespace DOG;

	auto e = m_entityManager.CreateEntity();
	auto e2 = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<DOG::TransformComponent>(e);
	m_entityManager.AddComponent<DOG::TransformComponent>(e2);

	m_entityManager.AddComponent<DOG::ModelComponent>(e);
	m_entityManager.AddComponent<DOG::ModelComponent>(e2);
}

void EmilDebugLayer::OnDetach()
{

}

void EmilDebugLayer::OnUpdate()
{
	
}

void EmilDebugLayer::OnRender()
{
	//...
}

void EmilDebugLayer::OnImGuiRender()
{
	//...
}

void EmilDebugLayer::OnEvent(DOG::IEvent&)
{
	
}
