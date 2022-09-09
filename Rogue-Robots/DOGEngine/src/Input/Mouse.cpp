#include "Mouse.h"
namespace DOG
{
	std::bitset<BUTTON_COUNT> Mouse::s_buttons;
	Vector2u Mouse::s_currentMouseCoords;
	Vector2i Mouse::s_deltaMouseCoords;

	void Mouse::OnButtonPressed(const Button button) noexcept
	{
		s_buttons[(u8)button] = true;
	}

	void Mouse::OnButtonReleased(const Button button) noexcept
	{
		s_buttons[(u8)button] = false;
	}

	void Mouse::OnMove(Vector2u newCoords) noexcept
	{
		s_currentMouseCoords = newCoords;
	}

	void Mouse::OnRawDelta(Vector2i deltaCoords) noexcept
	{
		s_deltaMouseCoords.x += deltaCoords.x;
		s_deltaMouseCoords.y += deltaCoords.y;
	}

	void Mouse::Reset() noexcept
	{
		s_deltaMouseCoords = { 0,0 };
	}

	const bool Mouse::IsButtonPressed(const Button button) noexcept
	{
		return s_buttons[(u8)button];
	}

	const std::pair<u32, u32> Mouse::GetCoordinates() noexcept
	{
		return std::make_pair(s_currentMouseCoords.x, s_currentMouseCoords.y);
	}

	const std::pair<i32, i32> Mouse::GetDeltaCoordinates() noexcept
	{
		return std::make_pair(s_deltaMouseCoords.x, s_deltaMouseCoords.y);
	}
}