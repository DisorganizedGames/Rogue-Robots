#include "GameLayer.h"
#include "Scripting/LuaEvent.h"
#include "Scripting/ScriptManager.h"

GameLayer::GameLayer() noexcept
	: Layer("Game layer")
{
	m_pipedData.viewMat = DirectX::XMMatrixLookAtLH({ 0.f, 5.f, 0.f }, { 0.f, 5.f, 1.f }, { 0.f, 1.f, 0.f });

	DOG::piper::SetPipe(&m_pipedData);
}

ScriptManager* scriptManager;

void GameLayer::OnAttach()
{
	//&LuaW::s_luaW.RunScript("EventSystem.lua");

	scriptManager = new ScriptManager(&LuaW::s_luaW);
	scriptManager->RunLuaFile("EventSystem.lua");
	scriptManager->AddScript("LuaTest.lua");

	m_debugCam = DebugCamera(0, 1, 0);
	LuaEvent::GetLuaEvent().Initialize(&LuaW::s_luaW, scriptManager);
	//LuaEvent::GetLuaEvent().AddListener("Damage", [](int g) {
	//		/*

	//			time where u get the args

	//			args_data = malloc(..)
	//			memcpy args into args_data

	//			map[name](args_data)

	//			map[name, std::function<void(void*)>
	//			
	//		*/
	//	});
	//LuaEvent::GetLuaEvent().AddListenerT<[](){}>("Damage");
	//LuaEvent::GetLuaEvent().AddListenerG("Damage", []() {});
}

void GameLayer::OnDetach()
{

}

void GameLayer::OnUpdate()
{
	m_debugCam.OnUpdate();
	m_pipedData.viewMat = m_debugCam.GetViewMatrix();
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
