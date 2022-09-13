#pragma once
#include "IEvent.h"
namespace DOG
{
	class WindowResizedEvent : public IEvent
	{
	public:
		explicit WindowResizedEvent(const Vector2u& newDimensions) noexcept : dimensions{ newDimensions }{}
		virtual ~WindowResizedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const noexcept override final { return EventType::WindowResizedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::WindowEventCategory; }
	public:
		Vector2u dimensions;
	};

	class WindowClosedEvent : public IEvent
	{
	public:
		explicit WindowClosedEvent() noexcept = default;
		virtual ~WindowClosedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const noexcept override final { return EventType::WindowClosedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::WindowEventCategory; }
	};
}