#include "EventPublisher.h"
namespace DOG
{
	void EventPublisher::PublishEvent(IEvent&& event) noexcept
	{
		EventBus::Get().Dispatch(std::move(event));
	}
}