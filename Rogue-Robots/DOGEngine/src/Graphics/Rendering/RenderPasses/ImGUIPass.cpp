#include "ImGUIPass.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ImGUIBackend.h"

#include "../RenderGraph/RenderGraph.h"
#include "../RenderGraph/RGBlackboard.h"

namespace DOG::gfx
{
	ImGUIPass::ImGUIPass(GlobalPassData& globalPassData, RGBlackboard& blackboard, ImGUIBackend* backend) :
		m_globalPassData(globalPassData),
		m_blackboard(blackboard),
		m_imgui(backend)
	{
	}

	void ImGUIPass::AddPass(RenderGraph& rg)
	{
		struct PassData {};
		rg.AddPass<PassData>("ImGUI Pass",
			[&](PassData&, RenderGraph::PassBuilder& builder)
			{
				builder.WriteRenderTarget(RG_RESOURCE(Backbuffer), RenderPassAccessType::PreservePreserve,
					TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
			},
			[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)
			{
				rd->Cmd_SetViewports(cmdl, m_globalPassData.bbVP);
				rd->Cmd_SetScissorRects(cmdl, m_globalPassData.bbScissor);
				m_imgui->Render(rd, cmdl);
			});
	}
}