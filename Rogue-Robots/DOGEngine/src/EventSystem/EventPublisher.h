#pragma once
#include "EventBuss.h"
#include "IEvent.h"
namespace DOG
{
	class EventPublisher
	{
	protected:
		template<typename EventType, typename ...Args>
		static void PublishEvent(Args&& ... args) noexcept;
		template<typename EventType>
		static void PublishEvent() noexcept;
		static void PublishEvent(IEvent&& event) noexcept;
		STATIC_CLASS(EventPublisher);
	};

	template<typename EventType, typename ...Args>
	void EventPublisher::PublishEvent(Args&& ... args) noexcept
	{
		EventBuss::Get().Dispatch(EventType({ std::forward<Args>(args)... }));
	}

	template<typename EventType>
	void EventPublisher::PublishEvent() noexcept
	{
		EventBuss::Get().Dispatch(EventType());
	}
}