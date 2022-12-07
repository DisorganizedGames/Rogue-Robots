#pragma once
#include "../EventSystem/EventPublisher.h"
namespace DOG
{
	enum class Key : u8 
	{
		BackSpace = 8, Tab,
		Enter = 13,
		Alt = 18,
		Capslock = 20,
		Esc = 27,
		Spacebar = 32,
		LeftArrow = 37, UpArrow, RightArrow, DownArrow,
		Zero = 48, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
		A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		F1 = 112, F2, F3, F4,
		LShift = 160, RShift, LCtrl, RCtrl,
		Period = 190//� = 192,
		// � = 221,
	};

	constexpr const u16 KEY_COUNT{ 256 };
	class Keyboard : public EventPublisher
	{
	public:
		static void OnKeyDown(const Key key) noexcept;
		static void OnKeyUp(const Key key) noexcept;
		static void Reset() noexcept;
		static [[nodiscard]] const bool IsKeyPressed(const Key key) noexcept;
	private:
		STATIC_CLASS(Keyboard);
	private:
		static std::bitset<KEY_COUNT> s_keys;
	};
}