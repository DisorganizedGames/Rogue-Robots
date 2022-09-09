#include "Keyboard.h"
namespace DOG
{
	std::bitset<KEY_COUNT> Keyboard::s_keys;
	void Keyboard::OnKeyDown(const Key key) noexcept
	{
		s_keys[(u8)key] = true;
	}

	void Keyboard::OnKeyUp(const Key key) noexcept
	{
		s_keys[(u8)key] = false;
	}

	const bool Keyboard::IsKeyPressed(const Key key) noexcept
	{
		return s_keys[(u8)key];
	}
}