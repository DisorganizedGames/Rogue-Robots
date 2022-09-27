#include "EmilFDebugLayer.h"
#include <StaticTypeInfo/type_id.h>
#include <StaticTypeInfo/type_index.h>
#include <StaticTypeInfo/type_name.h>

EmilFDebugLayer::EmilFDebugLayer() noexcept
	: Layer("Emil debug layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	
}

struct SpriteComponent
{

};

struct TestComponent
{

};

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
