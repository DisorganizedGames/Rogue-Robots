#pragma once
namespace DOG
{
	enum class EventCategory : u8 { WindowEventCategory = 0u, MouseEventCategory, KeyboardEventCategory };
	enum class EventType : u8 
	{ 
		WindowResizedEvent = 0u, WindowClosedEvent,
		LeftMouseButtonPressedEvent, LeftMouseButtonReleasedEvent, RightMouseButtonPressedEvent, RightMouseButtonReleasedEvent, MiddleMouseButtonPressedEvent, MiddleMouseButtonReleasedEvent,
		MouseMovedEvent,
		KeyPressedEvent, KeyReleasedEvent
	};

	class IEvent
	{
	public:
		IEvent() noexcept : m_donePropagating{false} {};
		virtual ~IEvent() noexcept = default;
		void StopPropagation() noexcept { m_donePropagating = true; }
		[[nodiscard]] constexpr const bool IsValid() const { return !m_donePropagating; }
		[[nodiscard]] virtual constexpr const EventType GetEventType() const = 0;
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const = 0;
	private:
		bool m_donePropagating;
	};

#ifndef EVENT
	#define EVENT(derived) static_cast<derived&>(event)
#endif
}