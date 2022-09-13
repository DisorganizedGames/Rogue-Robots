#pragma once
#include "IEvent.h"
namespace DOG
{
	class LeftMouseButtonPressedEvent : public IEvent
	{
	public:
		explicit LeftMouseButtonPressedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~LeftMouseButtonPressedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::LeftMouseButtonPressedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};

	class LeftMouseButtonReleasedEvent : public IEvent
	{
	public:
		explicit LeftMouseButtonReleasedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~LeftMouseButtonReleasedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::LeftMouseButtonReleasedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};

	class RightMouseButtonPressedEvent : public IEvent
	{
	public:
		explicit RightMouseButtonPressedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~RightMouseButtonPressedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::RightMouseButtonPressedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};

	class RightMouseButtonReleasedEvent : public IEvent
	{
	public:
		explicit RightMouseButtonReleasedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~RightMouseButtonReleasedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::RightMouseButtonReleasedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};

	class MiddleMouseButtonPressedEvent : public IEvent
	{
	public:
		explicit MiddleMouseButtonPressedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~MiddleMouseButtonPressedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::MiddleMouseButtonPressedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};

	class MiddleMouseButtonReleasedEvent : public IEvent
	{
	public:
		explicit MiddleMouseButtonReleasedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~MiddleMouseButtonReleasedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::MiddleMouseButtonReleasedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};

	class MouseMovedEvent : public IEvent
	{
	public:
		explicit MouseMovedEvent(const Vector2u& coordinates) noexcept : coordinates{ coordinates } {}
		virtual ~MouseMovedEvent() noexcept override final = default;
		[[nodiscard]] virtual constexpr const EventType GetEventType() const { return EventType::MouseMovedEvent; }
		[[nodiscard]] virtual constexpr const EventCategory GetEventCategory() const { return EventCategory::MouseEventCategory; }
	public:
		Vector2u coordinates;
	};
}
