#include "DX12RenderPass.h"

DX12RenderPass::DX12RenderPass(std::span<DX12RenderPass::RenderTarget> render_targets,
	std::optional<DX12RenderPass::DepthStencilTarget> ds_target, D3D12_RENDER_PASS_FLAGS)
{
	// Verifies any assumptions we have and assembles to API submittable form

	for (const auto& rt : render_targets)
	{
		m_render_targets.push_back(rt.get_desc());
	}

	if (ds_target)
	{
		m_depth_stencil = ds_target->get_desc();
	}
}

DX12RenderPass& DX12RenderPass::begin(ID3D12GraphicsCommandList4* cmdl)
{
	cmdl->BeginRenderPass(
		(UINT)m_render_targets.size(),
		m_render_targets.data(),
		m_depth_stencil ? &(*m_depth_stencil) : nullptr,
		m_flags);
	return *this;
}

void DX12RenderPass::end(ID3D12GraphicsCommandList4* cmdl)
{
	cmdl->EndRenderPass();
}
