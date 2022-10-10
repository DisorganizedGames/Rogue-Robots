#include "EmilFDebugLayer.h"
#include <ImGUI/imgui.h>

EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil F debug layer"), 
	  m_entityManager{ DOG::EntityManager::Get() }
{
}

void EmilFDebugLayer::OnAttach()
{
	auto& assetManager = DOG::AssetManager::Get();
	auto cube = assetManager.LoadModelAsset("Assets/red_cube.glb");
	cubeEntity = m_entityManager.CreateEntity();
	m_entityManager.AddComponent<DOG::ModelComponent>(cubeEntity, cube);
	m_entityManager.AddComponent<DOG::TransformComponent>(cubeEntity)
		.SetPosition({ 0.0f, 0.0f, 0.0f })
		.SetScale({ 1.0f, 1.0f, 1.0f })
		.SetRotation({ 0.0f, 0.0f, 0.0f });
}

void EmilFDebugLayer::OnDetach()
{

}

void EmilFDebugLayer::OnUpdate()
{
	//...
}

void EmilFDebugLayer::OnRender()
{
	//...
}

void EmilFDebugLayer::OnImGuiRender()
{
	auto& tc = m_entityManager.GetComponent<DOG::TransformComponent>(cubeEntity);

}

void EmilFDebugLayer::OnEvent(DOG::IEvent&)
{
	
}
