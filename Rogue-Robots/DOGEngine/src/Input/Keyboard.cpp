#include "Keyboard.h"
#include "../EventSystem/KeyboardEvents.h"
namespace DOG
{
	std::bitset<KEY_COUNT> Keyboard::s_keys;
	void Keyboard::OnKeyDown(const Key key) noexcept
	{
		s_keys[(u8)key] = true;
		PublishEvent<KeyPressedEvent>(key);
	}

	void Keyboard::OnKeyUp(const Key key) noexcept
	{
		s_keys[(u8)key] = false;
		PublishEvent<KeyReleasedEvent>(key);
	}

	void Keyboard::Reset() noexcept
	{
		for (int i = 0; i < KEY_COUNT; i++)
		{
			if (s_keys[i])
			{
				OnKeyUp((Key)i);
			}
		}
	}

	const bool Keyboard::IsKeyPressed(const Key key) noexcept
	{
		return s_keys[(u8)key];
	}
}