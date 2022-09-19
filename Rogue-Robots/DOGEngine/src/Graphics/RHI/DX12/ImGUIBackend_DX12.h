#pragma once
#include <d3d12.h>
#include "../ImGUIBackend.h"

namespace DOG::gfx
{
	class RenderDevice;
	class Swapchain;

	class ImGUIBackend_DX12 final : public ImGUIBackend
	{
	public:
		ImGUIBackend_DX12(RenderDevice* rd, Swapchain* sc, u32 maxFramesInFlight);
		~ImGUIBackend_DX12();

		// Public interface
		void BeginFrame();
		void EndFrame();
		void Render(RenderDevice* rd, CommandList cmdl);
		bool WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		// Implementation interface
	public:
		void Render(ID3D12GraphicsCommandList4* cmdl);

		/*
		
			// At m_rd->BeginFrame();
			ImGUI->BeginFrame();

			m_rd->Cmd_RenderImGUI(ImGUI)

			// At Renderer->EndFrame();
			ImGUI->EndFrame();
		
		*/

	private:

	};
}