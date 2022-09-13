#include "EventPublisher.h"
namespace DOG
{
	void EventPublisher::PublishEvent(IEvent&& event) noexcept
	{
		EventBuss::Get().Dispatch(std::move(event));
	}
}