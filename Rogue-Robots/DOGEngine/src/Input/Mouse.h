#pragma once
namespace DOG
{
	enum class Button : u8 {Left = 0u, Right, Wheel};
	constexpr const u16 BUTTON_COUNT{ 3 };
	class Mouse
	{
	public:
		static void OnButtonPressed(const Button button) noexcept;
		static void OnButtonReleased(const Button button) noexcept;
		static void OnMove(Vector2u newCoords) noexcept;
		static void OnRawDelta(Vector2i deltaCoords) noexcept;
		static void Reset() noexcept;
		static [[nodiscard]] const bool IsButtonPressed(const Button button) noexcept;
		static [[nodiscard]] const std::pair<u32, u32> GetCoordinates() noexcept;
		static [[nodiscard]] const std::pair<i32, i32> GetDeltaCoordinates() noexcept;
	private:
		static std::bitset<BUTTON_COUNT> s_buttons;
		static Vector2u s_currentMouseCoords;
		static Vector2i s_deltaMouseCoords;
		STATIC_CLASS(Mouse);
	};
}