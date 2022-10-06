#include "ImGUIPass.h"

#include "../RenderGraph/RenderGraph.h"
#include "../../RHI/ImGUIBackend.h"

namespace DOG::gfx
{
	ImGUIPass::ImGUIPass(ImGUIBackend* backend) :
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
				m_imgui->Render(rd, cmdl);
			});
	}
}