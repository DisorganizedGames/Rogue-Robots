#include "Mouse.h"
#include "../EventSystem/MouseEvents.h"
namespace DOG
{
	std::bitset<BUTTON_COUNT> Mouse::s_buttons;
	Vector2u Mouse::s_currentMouseCoords;
	Vector2i Mouse::s_deltaMouseCoords[2];
	u8 Mouse::s_deltaMouseCoordsIndex = 0;

	void Mouse::OnButtonPressed(const Button button) noexcept
	{
		s_buttons[(u8)button] = true;
		switch (button)
		{
		case Button::Left: PublishEvent<LeftMouseButtonPressedEvent>(s_currentMouseCoords); break;
		case Button::Right: PublishEvent<RightMouseButtonPressedEvent>(s_currentMouseCoords); break;
		case Button::Wheel: PublishEvent<MiddleMouseButtonPressedEvent>(s_currentMouseCoords); break;
		}
	}

	void Mouse::OnButtonReleased(const Button button) noexcept
	{
		s_buttons[(u8)button] = false;
		switch (button)
		{
		case Button::Left: PublishEvent<LeftMouseButtonReleasedEvent>(s_currentMouseCoords); break;
		case Button::Right: PublishEvent<RightMouseButtonReleasedEvent>(s_currentMouseCoords); break;
		case Button::Wheel: PublishEvent<MiddleMouseButtonReleasedEvent>(s_currentMouseCoords); break;
		}
	}

	void Mouse::OnMove(Vector2u newCoords) noexcept
	{
		s_currentMouseCoords = newCoords;
		PublishEvent<MouseMovedEvent>(s_currentMouseCoords);
	}

	void Mouse::OnRawDelta(Vector2i deltaCoords) noexcept
	{
		s_deltaMouseCoords[s_deltaMouseCoordsIndex].x += deltaCoords.x;
		s_deltaMouseCoords[s_deltaMouseCoordsIndex].y += deltaCoords.y;
	}

	void Mouse::Reset() noexcept
	{
		s_deltaMouseCoords[s_deltaMouseCoordsIndex] = {0,0};
	}

	void Mouse::Switch() noexcept
	{
		s_deltaMouseCoordsIndex = (s_deltaMouseCoordsIndex + 1) % 2;
	}

	const bool Mouse::IsButtonPressed(const Button button) noexcept
	{
		return s_buttons[(u8)button];
	}

	const std::pair<u32, u32> Mouse::GetCoordinates() noexcept
	{
		return { s_currentMouseCoords.x, s_currentMouseCoords.y };
	};

	const std::pair<i32, i32> Mouse::GetDeltaCoordinates() noexcept
	{
		u8 index = (s_deltaMouseCoordsIndex + 1) % 2;
		return { s_deltaMouseCoords[index].x, s_deltaMouseCoords[index].y};
	}
}