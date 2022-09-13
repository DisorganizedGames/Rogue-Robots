#include "EventBuss.h"
#include "IEvent.h"
#include "LayerStack.h"
namespace DOG
{
	EventBuss EventBuss::s_instance;
	
	void EventBuss::Dispatch(IEvent&& event) const noexcept
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

	void EventBuss::SetMainApplication(Application* application) noexcept
	{
		ASSERT(application, "Application pointer is invalid.");
		m_mainApplication = application;
	}
}