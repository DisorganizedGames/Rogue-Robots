#pragma once
#include "../CommonIncludes_DX12.h"
#include <optional>
#include <array>
#include <span>
#include <vector>

class DX12RenderPass
{
	// Helper structs
public:
	// To implement later (we wont support resolving for now)
	class ResolveMetadata
	{
	public:
		operator D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_PARAMETERS() const { return D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_PARAMETERS(); }


	private:
	};

	class RenderTarget
	{
	public:
		RenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE handle) { assert(handle.ptr != 0); m_desc.cpuDescriptor = handle; }
		RenderTarget& set_begin_access_type(D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE access_type) { m_desc.BeginningAccess.Type = access_type; return *this; }
		RenderTarget& set_end_access_type(D3D12_RENDER_PASS_ENDING_ACCESS_TYPE access_type) { m_desc.EndingAccess.Type = access_type; return *this; };

		// Optional (only needed if Begin Access == CLEAR
		RenderTarget& set_begin_clear(const D3D12_CLEAR_VALUE& clear_value) { m_desc.BeginningAccess.Clear.ClearValue = clear_value; return *this; };

		// Optional (only needed if End Access == RESOLVE)
		RenderTarget& set_end_resolve(const DX12RenderPass::ResolveMetadata& resolve_md) { m_desc.EndingAccess.Resolve = resolve_md; return *this; };

		D3D12_RENDER_PASS_RENDER_TARGET_DESC get_desc() const { return m_desc; }

	private:
		D3D12_RENDER_PASS_RENDER_TARGET_DESC m_desc{};

	};

	class DepthStencilTarget
	{
	public:
		DepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE handle) { assert(handle.ptr != 0); m_desc.cpuDescriptor = handle; }
		DepthStencilTarget& set_depth_begin_access_type(D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE access) { m_desc.DepthBeginningAccess.Type = access; return *this; }
		DepthStencilTarget& set_depth_end_access_type(D3D12_RENDER_PASS_ENDING_ACCESS_TYPE access) { m_desc.DepthEndingAccess.Type = access; return *this; };
		DepthStencilTarget& set_stencil_begin_access_type(D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE access) { m_desc.StencilBeginningAccess.Type = access; return *this; }
		DepthStencilTarget& set_stencil_end_access_type(D3D12_RENDER_PASS_ENDING_ACCESS_TYPE access) { m_desc.StencilEndingAccess.Type = access; return *this; };

		// Optional (only needed if Begin Access == CLEAR)
		DepthStencilTarget& set_depth_clear(const D3D12_CLEAR_VALUE& clear_value) { m_desc.DepthBeginningAccess.Clear.ClearValue = clear_value; return *this; }
		DepthStencilTarget& set_stencil_clear(const D3D12_CLEAR_VALUE& clear_value) { m_desc.StencilBeginningAccess.Clear.ClearValue = clear_value; return *this; }

		// Optional (only needed if End Access == RESOLVE)
		DepthStencilTarget& set_depth_resolve(const ResolveMetadata& resolve_md) { m_desc.DepthEndingAccess.Resolve = resolve_md; return *this; }
		DepthStencilTarget& set_stencil_resolve(const ResolveMetadata& resolve_md) { m_desc.StencilEndingAccess.Resolve = resolve_md; return *this; }

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC get_desc() const { return m_desc; }

	private:
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_desc{};

	};

public:
	DX12RenderPass(
		std::span<DX12RenderPass::RenderTarget> render_targets,
		const std::optional<DX12RenderPass::DepthStencilTarget> ds_target = {},
		D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE
	);

	DX12RenderPass& begin(ID3D12GraphicsCommandList4* cmdl);
	void end(ID3D12GraphicsCommandList4* cmdl);

private:
	std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> m_render_targets;
	std::optional<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> m_depth_stencil;
	D3D12_RENDER_PASS_FLAGS m_flags{};


};

