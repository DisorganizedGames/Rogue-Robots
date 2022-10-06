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

	class WindowActiveEvent : public IEvent
	{
	public:
		explicit WindowActiveEvent(bool active) noexcept : active{ active }{};
		virtual ~WindowActiveEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const noexcept override final { return EventType::WindowActiveEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::WindowEventCategory; }
	public:
		bool active;
	};

	class WindowHitBorderEvent : public IEvent
	{
	public:
		explicit WindowHitBorderEvent() noexcept = default;
		virtual ~WindowHitBorderEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const noexcept override final { return EventType::WindowHitBorderEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::WindowEventCategory; }
	};

	class WindowAltEnterEvent : public IEvent
	{
	public:
		explicit WindowAltEnterEvent() noexcept = default;
		virtual ~WindowAltEnterEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const noexcept override final { return EventType::WindowAltEnterEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::WindowEventCategory; }
	};
}