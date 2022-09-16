#pragma once
#include "../RenderBackend.h"
#include "CommonIncludes_DX12.h"
#include <unordered_map>

/*
	There should be Device left alive after debug print! :)
	It will be cleaned up right after the print.
*/

namespace DOG::gfx
{
	class RenderBackend_DX12 final : public RenderBackend
	{
	public:
		RenderBackend_DX12(bool debug = false);

		RenderDevice* CreateDevice();

	private:
		void CreateAdapterFac();
		void SelectAdapter();
		void CheckFeatureSupport();

		struct FinalDebug
		{
			~FinalDebug();
		};

	private:
		bool m_debug_on{ false };

		
		std::unordered_map<RenderDevice*, ComPtr<ID3D12Device>> m_devices;
		FinalDebug m_debugPrint;
		std::unordered_map<RenderDevice*, std::unique_ptr<RenderDevice>> m_renderDevices;


		ComPtr<IDXGIFactory6> m_dxgiFac;
		ComPtr<IDXGIAdapter> m_adapter;
		DXGI_ADAPTER_DESC m_adapterDesc;

	};
}