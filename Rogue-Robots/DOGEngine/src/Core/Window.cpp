#include "Window.h"
#include "Application.h"
#include "../Input/Keyboard.h"
#include "../Input/Mouse.h"
#include "../EventSystem/WindowEvents.h"
#include <ImGUI/imgui.h>
namespace DOG
{
	void MapLeftRightShiftAndControl(WPARAM& wParam, LPARAM lParam);

	struct WindowData
	{
		HWND windowHandle;
		RECT windowRectangle;
		Vector2u dimensions;
		WindowMode mode;
		CursorMode cursorMode = CursorMode::Visible;
		ImFont* imguiFont;
	};
	static WindowData s_windowData = {};
	static std::optional<std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>> s_wmHook;

	LRESULT Window::WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		MINIPROFILE
		if (s_wmHook)
		{

			switch (message)
			{
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_INPUT:
			{
				if ((Window::GetCursorMode() & CursorMode::Visible) != CursorMode::Visible)
				{
					break;
				}
			}
			default:
				if ((*s_wmHook)(windowHandle, message, wParam, lParam))
				{
					return true;
				}
			}
		}

		switch (message)
		{
		case WM_CLOSE:
		{
			// DOG::Application listens for WindowClosedEvent, it will call DestroyWindow if WindowClosedEvent is not stopped by another listener.
			PublishEvent<WindowClosedEvent>();
			return 0;
		}
		case WM_DESTROY:
		{
			// At this point the window will be killed, no going back.
			PostQuitMessage(0);
			return 0;
		}
		case WM_WINDOWPOSCHANGING:
		{
			if (IsMinimized(Window::GetHandle())) return 0;
			PublishEvent<WindowPosChangingEvent>();
			break;
		}
		case WM_SIZE:
		{
			if(IsMinimized(Window::GetHandle())) return 0;

			s_windowData.dimensions.x = LOWORD(lParam);
			s_windowData.dimensions.y = HIWORD(lParam);

			PublishEvent<WindowResizedEvent>(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
		case WM_MOUSEACTIVATE:
		{
			if (LOWORD(lParam) == HTTOP || LOWORD(lParam) == HTBOTTOM || LOWORD(lParam) == HTLEFT || LOWORD(lParam) == HTRIGHT ||
				LOWORD(lParam) == HTCLOSE || LOWORD(lParam) == HTMAXBUTTON || LOWORD(lParam) == HTMINBUTTON)
			{
				PublishEvent<WindowHitBorderEvent>();
				return MA_NOACTIVATE;
			}
			return MA_ACTIVATEANDEAT;
		}
		case WM_ACTIVATEAPP:
		{
			PublishEvent<WindowActiveEvent>(static_cast<bool>(wParam));
			return 0;
		}
		case WM_SYSKEYDOWN:
		{
			bool keyIsRepeated = (lParam >> 30) & 1;
			if (wParam == VK_RETURN && (lParam & (1 << 29) && !keyIsRepeated))
			{
				PublishEvent<WindowAltEnterEvent>(); // Handle ALT+ENTER:
			}
			if (wParam == VK_F4 && (lParam & (1 << 29) && !keyIsRepeated))
			{
				PublishEvent<WindowClosedEvent>();  // Handle ALT+F4
			}
		}
		case WM_KEYDOWN:
		{
			bool keyIsRepeated = (lParam >> 30) & 1;
			if (!keyIsRepeated)
			{
				MapLeftRightShiftAndControl(wParam, lParam);
				Keyboard::OnKeyDown((Key)(u8)(wParam));
			}
			return 0;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			MapLeftRightShiftAndControl(wParam, lParam);
			Keyboard::OnKeyUp((Key)(u8)(wParam));
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			Mouse::OnButtonPressed(Button::Left);
			return 0;
		}
		case WM_LBUTTONUP:
		{
			Mouse::OnButtonReleased(Button::Left);
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			Mouse::OnButtonPressed(Button::Right);
			return 0;
		}
		case WM_RBUTTONUP:
		{
			Mouse::OnButtonReleased(Button::Right);
			return 0;
		}
		case WM_MBUTTONDOWN:
		{
			Mouse::OnButtonPressed(Button::Wheel);
			return 0;
		}
		case WM_MBUTTONUP:
		{
			Mouse::OnButtonReleased(Button::Wheel);
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			// LOWORD and HIWORD can't be used because x and y can be negative
			POINTS point = MAKEPOINTS(lParam);
			if (point.x < 0 || point.y < 0 || point.x >= static_cast<int>(GetWidth()) || point.y >= static_cast<int>(GetHeight()))
			{
				std::cout << "Warning WM_MOUSEMOVE returned point outside the windows dimensions! x: " << point.x << " y: " << point.y << std::endl;
			}
			Mouse::OnMove({ static_cast<u32>(point.x), static_cast<u32>(point.y) });
			return 0;
		}
		case WM_INPUT:
		{
			UINT size = 0u;
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == -1)
				return 0;

			std::vector<char> rawBuffer;
			rawBuffer.resize(size);

			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawBuffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
				return 0;

			auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.data());
			if (ri.header.dwType == RIM_TYPEMOUSE && (ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				Mouse::OnRawDelta({ ri.data.mouse.lLastX, ri.data.mouse.lLastY });
			}

			return 0;
		}
		}
		return DefWindowProcA(windowHandle, message, wParam, lParam);
	}

	void Window::Initialize(const ApplicationSpecification& spec) noexcept
	{
		ASSERT((spec.windowDimensions.x > 0 && spec.windowDimensions.y > 0)
			,"Window dimensions are invalid.");

		s_windowData.dimensions.x = spec.windowDimensions.x;
		s_windowData.dimensions.y = spec.windowDimensions.y;
		s_windowData.mode = spec.initialWindowMode;
		
		std::string className = spec.name + "Class";
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
		ASSERT_FUNC(::RegisterClassExA(&windowClass), "Failed to register Window class.");

		RECT windowRectangle = {0u, 0u, static_cast<LONG>(spec.windowDimensions.x), static_cast<LONG>(spec.windowDimensions.y)};
		//ASSERT_FUNC(::AdjustWindowRect(&windowRectangle, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME, FALSE), "Failed to adjust window rectangle.");
		ASSERT_FUNC(::AdjustWindowRect(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE), "Failed to adjust window rectangle.");
	
		DEVMODEA devMode = {};
		devMode.dmSize = sizeof(DEVMODE);
		ASSERT_FUNC(::EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &devMode), "Failed to enumerate display settings.");
		int windowCenterPosX = (devMode.dmPelsWidth / 2) - static_cast<int>((spec.windowDimensions.x / 2));
		int windowCenterPosY = (devMode.dmPelsHeight / 2) - static_cast<int>((spec.windowDimensions.y / 2));

		s_windowData.windowHandle = ::CreateWindowExA
		(
			0u,												//DwExStyle
			className.c_str(),								//Class name
			spec.name.c_str(),								//Window name
			//WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,			//Window styles
			WS_OVERLAPPEDWINDOW,			//Window styles
			windowCenterPosX,								//Window center X
			windowCenterPosY,								//Window center Y
			windowRectangle.right - windowRectangle.left,	//Width
			windowRectangle.bottom - windowRectangle.top,	//Height
			nullptr,										//Parent window
			nullptr,										//Menu
			::GetModuleHandleA(nullptr),					//Instance
			nullptr											//Data passed along
		);
		s_windowData.windowRectangle = windowRectangle;

		//Register raw mouse input:
		RAWINPUTDEVICE rawInputDevice = {};
		rawInputDevice.usUsagePage = 0x01;
		rawInputDevice.usUsage = 0x02;
		rawInputDevice.dwFlags = 0;
		rawInputDevice.hwndTarget = nullptr;
		ASSERT_FUNC(::RegisterRawInputDevices(&rawInputDevice, 1u, sizeof(rawInputDevice)) 
			,"Failed to register raw mouse movement delta.");

		::ShowWindow(s_windowData.windowHandle, SW_SHOW);
	}

	bool Window::OnUpdate() noexcept
	{
		MSG message = {};
		while (::PeekMessageA(&message, nullptr, 0u, 0u, PM_REMOVE))
		{
			if (message.message == WM_QUIT)
			{
				return false;
			}
			::TranslateMessage(&message);
			::DispatchMessageA(&message);
		}
		return true;
	}

	const u32 Window::GetWidth() noexcept
	{
		return s_windowData.dimensions.x;
	}

	const u32 Window::GetHeight() noexcept
	{
		return s_windowData.dimensions.y;
	}

	const std::pair<u32, u32> Window::GetDimensions() noexcept
	{
		return std::make_pair(s_windowData.dimensions.x, s_windowData.dimensions.y);
	}

	const WindowMode Window::GetMode() noexcept
	{
		return s_windowData.mode;
	}

	const RECT Window::GetWindowRect() noexcept
	{
		RECT r;
		::GetWindowRect(s_windowData.windowHandle, &r);
		return r;
	}

	const HWND Window::GetHandle() noexcept
	{
		return s_windowData.windowHandle;
	}

	void Window::SetWMHook(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> func)
	{
		s_wmHook = func;
	}

	void Window::SetCursorMode(CursorMode mode) noexcept
	{
		if ((s_windowData.cursorMode & CursorMode::Visible) != (mode & CursorMode::Visible))
		{
			if ((mode & CursorMode::Visible) == CursorMode::Visible)
				ShowCursor(true);
			else
				ShowCursor(false);
		}
		if ((s_windowData.cursorMode & CursorMode::Confined) != (mode & CursorMode::Confined))
		{
			if ((mode & CursorMode::Confined) == CursorMode::Confined)
			{
				RECT r;
				GetClientRect(s_windowData.windowHandle, &r);
				MapWindowPoints(s_windowData.windowHandle, nullptr, (POINT*)&r, 2);
				ClipCursor(&r);
			}
			else
			{
				ClipCursor(nullptr);
			}
		}
		s_windowData.cursorMode = mode;
	}

	CursorMode Window::GetCursorMode() noexcept
	{
		return s_windowData.cursorMode;
	}

	void Window::SetFont(ImFont* pFont) noexcept
	{
		s_windowData.imguiFont = pFont;
	}

	ImFont* Window::GetFont() noexcept
	{
		ASSERT(s_windowData.imguiFont, "Font pointer is not valid.");
		return s_windowData.imguiFont;
	}

	void MapLeftRightShiftAndControl(WPARAM& wParam, LPARAM lParam)
	{
		if (wParam == VK_SHIFT)
			wParam = MapVirtualKey((lParam & 0xff0000) >> 16, MAPVK_VSC_TO_VK_EX);
		else if (wParam == VK_CONTROL)
			wParam = lParam & 0x01000000 ? VK_RCONTROL : VK_LCONTROL;
	}
}