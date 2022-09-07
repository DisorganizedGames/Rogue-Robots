#include "Window.h"
#include "Application.h"
namespace DOG
{
	struct WindowData
	{
		Vector2u Dimensions;
	};
	static WindowData s_WindowData;

	LRESULT WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{

	}

	void Window::Initialize(const ApplicationSpecification& spec) noexcept
	{
		assert((spec.WindowDimensions.x > 0 && spec.WindowDimensions.y > 0) 
			&& "Window dimensions are invalid.");
		
		std::string className = spec.Name + "Class";
		WNDCLASSEXA windowClass = {};
		windowClass.cbSize = sizeof(WNDCLASSEXA);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProcedure;
		windowClass.cbClsExtra = 0u;
		windowClass.cbWndExtra = 0u;
		windowClass.hInstance = ::GetModuleHandleA(nullptr);
		windowClass.hIcon = ::LoadCursorA(nullptr, IDC_ARROW);
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName = nullptr;
		windowClass.lpszClassName = className.c_str();
		windowClass.hIconSm = nullptr;
		assert(::RegisterClassExA(&windowClass) != 0 && "Failed to register Window class.");

		HWND windowHandle = ::CreateWindowExA
		(
			0u,						//dwExStyle
			className.c_str(),		//className
			spec.Name.c_str(),		//windowName
			WS_OVERLAPPEDWINDOW,	//WindowStyles


		);
	}
}