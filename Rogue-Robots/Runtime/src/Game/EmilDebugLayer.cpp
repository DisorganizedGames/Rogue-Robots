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
	auto& t = m_entityManager.AddComponent<DOG::TransformComponent>(e);
	m_entityManager.AddComponent<DOG::TransformComponent>(e2);
	t.m_position.x = 10.0f;
	t.m_position.y = 10.0f;
	t.m_position.z = 10.0f;

	m_entityManager.AddComponent<DOG::ModelComponent>(e);
	m_entityManager.AddComponent<DOG::ModelComponent>(e2);

	m_entityManager.Collect<TransformComponent, ModelComponent>().Do([](TransformComponent& t, ModelComponent& m)
		{
			t.m_position.x = 0.0f;
			std::cout << m.id;
		});

	m_entityManager.Collect<TransformComponent, ModelComponent>().Do([](entity id, TransformComponent& t, ModelComponent& m)
		{
			t.m_position.x = 0.0f;
			EntityManager::Get().RemoveComponent<TransformComponent>(id);
			std::cout << id;
			std::cout << m.id;
		});

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

//Place-holder example on how to use event system:
void EmilDebugLayer::OnEvent(DOG::IEvent&)
{
	
}
