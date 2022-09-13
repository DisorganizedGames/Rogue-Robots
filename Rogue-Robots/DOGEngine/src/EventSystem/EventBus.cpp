#include "EventBus.h"
#include "IEvent.h"
#include "LayerStack.h"
namespace DOG
{
	EventBus EventBus::s_instance;
	
	void EventBus::Dispatch(IEvent&& event) const noexcept
	{
		for (auto layer = LayerStack::Get().end(); layer != LayerStack::Get().begin();)
		{
			if (!event.IsValid())
				return;
	
			(*--layer)->OnEvent(event);
		}
	
		//The main application get to respond to the event last:
		m_mainApplication->OnEvent(event);
	}

	void EventBus::SetMainApplication(Application* application) noexcept
	{
		ASSERT(application, "Application pointer is invalid.");
		m_mainApplication = application;
	}
}