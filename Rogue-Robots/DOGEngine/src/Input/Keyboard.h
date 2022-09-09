#pragma once
namespace DOG
{
	enum class Key : u8 
	{
		BackSpace = 8, Tab, Enter = 13, Shift = 16, Ctrl, AltGr, Capslock = 20, Spacebar = 32,
		LeftArrow = 37, UpArrow, RightArrow, DownArrow,
		Zero = 48, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
		A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		Ö = 192, Å = 221, Ä
	};

	constexpr const u16 KEY_COUNT{ 256 };
	class Keyboard
	{
	public:
		static void OnKeyDown(const Key key) noexcept;
		static void OnKeyUp(const Key key) noexcept;
		static [[nodiscard]] const bool IsKeyPressed(const Key key) noexcept;
	private:
		STATIC_CLASS(Keyboard);
	private:
		static std::bitset<KEY_COUNT> s_keys;
	};
}