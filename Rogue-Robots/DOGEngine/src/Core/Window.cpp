#include "Window.h"
#include "Application.h"
#include "../Input/Keyboard.h"
#include "../Input/Mouse.h"
namespace DOG
{
	struct WindowData
	{
		HWND WindowHandle;
		RECT WindowRectangle;
		Vector2u Dimensions;
		WindowMode Mode;
	};
	static WindowData s_windowData = {};

	LRESULT WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
		{
			::PostQuitMessage(0);
			break;
		}
		case WM_SIZE:
		{
			s_windowData.Dimensions.x = LOWORD(lParam);
			s_windowData.Dimensions.y = HIWORD(lParam);
			break;
		}
		case WM_KEYDOWN:
		{
			bool keyIsRepeated = (lParam >> 30) & 1;
			if (!keyIsRepeated)
			{
				Keyboard::OnKeyDown((Key)(u8)(wParam));
			}
			break;
		}
		case WM_KEYUP:
		{
			Keyboard::OnKeyUp((Key)(u8)(wParam));
			break;
		}
		case WM_LBUTTONDOWN:
		{
			Mouse::OnButtonPressed(Button::Left);
			break;
		}
		case WM_LBUTTONUP:
		{
			Mouse::OnButtonReleased(Button::Left);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			Mouse::OnButtonPressed(Button::Right);
			break;
		}
		case WM_RBUTTONUP:
		{
			Mouse::OnButtonReleased(Button::Right);
			break;
		}
		case WM_MBUTTONDOWN:
		{
			Mouse::OnButtonPressed(Button::Wheel);
			break;
		}
		case WM_MBUTTONUP:
		{
			Mouse::OnButtonReleased(Button::Wheel);
			break;
		}
		case WM_MOUSEMOVE:
		{
			Mouse::OnMove({ LOWORD(lParam), HIWORD(lParam) });
			break;
		}
		case WM_INPUT:
		{
			UINT size = 0u;
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == -1)
				break;

			std::vector<char> rawBuffer;
			rawBuffer.resize(size);

			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawBuffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
				break;

			auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.data());
			if (ri.header.dwType == RIM_TYPEMOUSE && (ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				Mouse::OnRawDelta({ ri.data.mouse.lLastX, ri.data.mouse.lLastY });
			}

			break;
		}
		}
		return DefWindowProcA(windowHandle, message, wParam, lParam);
	}

	void Window::Initialize(const ApplicationSpecification& spec) noexcept
	{
		assert((spec.WindowDimensions.x > 0 && spec.WindowDimensions.y > 0) 
			&& "Window dimensions are invalid.");

		s_windowData.Dimensions.x = spec.WindowDimensions.x;
		s_windowData.Dimensions.y = spec.WindowDimensions.y;
		s_windowData.Mode = spec.InitialWindowMode;
		
		std::string className = spec.Name + "Class";
		WNDCLASSEXA windowClass = {};
		windowClass.cbSize = sizeof(WNDCLASSEXA);				//Size of struct
		windowClass.style = CS_HREDRAW | CS_VREDRAW;			//Window styles
		windowClass.lpfnWndProc = WindowProcedure;				//Message function
		windowClass.cbClsExtra = 0u;							//Extra bytes following structure
		windowClass.cbWndExtra = 0u;							//Extra bytes following window
		windowClass.hInstance = ::GetModuleHandleA(nullptr);	//The program instance
		windowClass.hIcon = ::LoadCursor(nullptr, IDC_ARROW);	//Window icon
		windowClass.hbrBackground = nullptr;					//Background brush handle
		windowClass.lpszMenuName = nullptr;						//Menu name
		windowClass.lpszClassName = className.c_str();			//Class name
		windowClass.hIconSm = nullptr;							//Small icon
		assert(::RegisterClassExA(&windowClass) != 0 && "Failed to register Window class.");

		RECT windowRectangle = {0u, 0u, static_cast<LONG>(spec.WindowDimensions.x), static_cast<LONG>(spec.WindowDimensions.y)};
		assert(::AdjustWindowRect(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE) != 0 && "Failed to adjust window rectangle.");

		DEVMODEA devMode = {};
		devMode.dmSize = sizeof(DEVMODE);
		assert(::EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &devMode) && "Failed to enumerate display settings.");
		int windowCenterPosX = (devMode.dmPelsWidth / 2) - static_cast<int>((spec.WindowDimensions.x / 2));
		int windowCenterPosY = (devMode.dmPelsHeight / 2) - static_cast<int>((spec.WindowDimensions.y / 2));

		s_windowData.WindowHandle = ::CreateWindowExA
		(
			0u,												//DwExStyle
			className.c_str(),								//Class name
			spec.Name.c_str(),								//Window name
			WS_OVERLAPPEDWINDOW,							//Window styles
			windowCenterPosX,								//Window center X
			windowCenterPosY,								//Window center Y
			windowRectangle.right - windowRectangle.left,	//Width
			windowRectangle.bottom - windowRectangle.top,	//Height
			nullptr,										//Parent window
			nullptr,										//Menu
			::GetModuleHandleA(nullptr),					//Instance
			nullptr											//Data passed along
		);
		s_windowData.WindowRectangle = windowRectangle;

		//Register raw mouse input:
		RAWINPUTDEVICE rawInputDevice = {};
		rawInputDevice.usUsagePage = 0x01;
		rawInputDevice.usUsage = 0x02;
		rawInputDevice.dwFlags = 0;
		rawInputDevice.hwndTarget = nullptr;
		assert(::RegisterRawInputDevices(&rawInputDevice, 1u, sizeof(rawInputDevice)) 
			&& "Failed to register raw mouse movement delta.");

		::ShowWindow(s_windowData.WindowHandle, SW_SHOW);
	}

	bool Window::OnUpdate() noexcept
	{
		MSG message = {};
		while (::PeekMessageA(&message, nullptr, 0u, 0u, PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessageA(&message);

			if (message.message == WM_QUIT)
				return false;
		}
		return true;
	}

	const u32 Window::GetWidth() noexcept
	{
		return s_windowData.Dimensions.x;
	}

	const u32 Window::GetHeight() noexcept
	{
		return s_windowData.Dimensions.y;
	}

	const std::pair<u32, u32> GetDimensions() noexcept
	{
		return std::make_pair(s_windowData.Dimensions.x, s_windowData.Dimensions.y);
	}

	const WindowMode Window::GetMode() noexcept
	{
		return s_windowData.Mode;
	}
}