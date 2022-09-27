#include "EmilFDebugLayer.h"

EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil debug layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	
}

struct SpriteComponent : public DOG::Component<SpriteComponent>
{

};

void EmilFDebugLayer::OnAttach()
{
	using namespace DOG;

	auto& am = DOG::AssetManager::Get();
	m_redCube = am.LoadModelAsset("Assets/red_cube.glb");
	m_greenCube = am.LoadModelAsset("Assets/green_cube.glb");
	m_blueCube = am.LoadModelAsset("Assets/blue_cube.glb");
	m_magentaCube = am.LoadModelAsset("Assets/magenta_cube.glb");



	for (int i = 0; i < 10; i++)
	{
		entity id = m_entityManager.CreateEntity();
		if (i != 8)
		{
			m_entityManager.AddComponent<TransformComponent>(id)
				.SetPosition({ 4, -2, 5 })
				.SetRotation({ 3.14f / 4.0f, 0, 0 })
				.SetScale({0.5, 0.5, 0.5});
		}
		if (id % 2 == 0)
		{
			m_entityManager.AddComponent<ModelComponent>(id, m_redCube);
		}
	}
	for (int i = 0; i < 3; i++)
	{
		m_entityManager.RemoveComponent<TransformComponent>(i);
	}

	m_entityManager.AddComponent<TransformComponent>(8, DirectX::SimpleMath::Vector3{ 50.0f, 50.0f, 50.0f });

	//
	//auto newE = m_entityManager.CreateEntity();
	//auto& w2 = m_entityManager.AddComponent<TransformComponent>(newE, DirectX::SimpleMath::Vector3{ 50.0f, 50.0f, 500.0f });
	//std::cout << "HEHE";


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
