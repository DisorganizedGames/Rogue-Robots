#include "EmilDebugLayer.h"

EmilDebugLayer::EmilDebugLayer() noexcept
	: Layer("Emil debug layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	
}

struct SpriteComponent : public DOG::Component<SpriteComponent>
{

};

void EmilDebugLayer::OnAttach()
{
	using namespace DOG;

	//auto e = m_entityManager.CreateEntity();
	//auto e2 = m_entityManager.CreateEntity();
	//m_entityManager.AddComponent<DOG::TransformComponent>(e);
	//m_entityManager.AddComponent<DOG::TransformComponent>(e2);


	//m_entityManager.AddComponent<DOG::ModelComponent>(e);
	//m_entityManager.AddComponent<DOG::ModelComponent>(e2);

	for (int i = 0; i < 10; i++)
	{
		entity id = m_entityManager.CreateEntity();
		m_entityManager.AddComponent<TransformComponent>(id);
		if (id % 2 == 0)
		{
			m_entityManager.AddComponent<SpriteComponent>(id);
		}
	}
	for (int i = 0; i < 3; i++)
	{
		m_entityManager.RemoveComponent<TransformComponent>(i);
	}

	m_entityManager.Bundlee<TransformComponent, SpriteComponent>();
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
