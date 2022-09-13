#pragma once
#include "IEvent.h"
namespace DOG
{
	class KeyPressedEvent : public IEvent
	{
	public:
		explicit KeyPressedEvent(Key key) noexcept : key{key}{}
		virtual ~KeyPressedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::KeyPressedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::KeyboardEventCategory; }
	public:
		Key key;
	};

	class KeyReleasedEvent : public IEvent
	{
	public:
		explicit KeyReleasedEvent(Key key) noexcept : key{ key } {}
		virtual ~KeyReleasedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::KeyReleasedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::KeyboardEventCategory; }
	public:
		Key key;
	};
}
