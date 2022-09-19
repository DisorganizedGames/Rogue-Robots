#include "ImGUIBackend_DX12.h"

#include "ImGUI/imgui.h"
#include "ImGUI/backends/imgui_impl_dx12.h"
#include "ImGUI/backends/imgui_impl_win32.h"

#include "RenderDevice_DX12.h"
#include "Swapchain_DX12.h"


DOG::gfx::ImGUIBackend_DX12::ImGUIBackend_DX12(RenderDevice* rd, Swapchain* sc, u32 maxFramesInFlight)
{
	// This is guaranteed since we have no other backends than DX12
	auto rd12 = (RenderDevice_DX12*)rd;
	auto sc12 = (Swapchain_DX12*)sc;
	
	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.FontGlobalScale = 1.f;
	//io.ConfigDockingWithShift = true;

	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(sc12->GetHWND());
	ImGui_ImplDX12_Init(
		rd12->GetDevice(),
		maxFramesInFlight + 1, sc12->GetBufferFormat(),	// ImGUI requires 2 frames in flight at least for some reason.
		rd12->GetMainResourceDH(),
		rd12->GetReservedResourceHandle().first,
		rd12->GetReservedResourceHandle().second);

}

DOG::gfx::ImGUIBackend_DX12::~ImGUIBackend_DX12()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void DOG::gfx::ImGUIBackend_DX12::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	/*
		Make dockspace over the whole viewport
		This has to be called prior to all other ImGUI calls
	*/
	ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;      // Pass through the otherwise background cleared window
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), flags);
}

void DOG::gfx::ImGUIBackend_DX12::Render(ID3D12GraphicsCommandList4* cmdl)
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdl);
}

void DOG::gfx::ImGUIBackend_DX12::EndFrame()
{
	ImGuiIO& io = ImGui::GetIO();

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void DOG::gfx::ImGUIBackend_DX12::Render(RenderDevice* rd, CommandList cmdl)
{
	auto rd12 = (RenderDevice_DX12*)rd;
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), rd12->GetListForExternal(cmdl));
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool DOG::gfx::ImGUIBackend_DX12::WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
		return true;
	return false;
}
