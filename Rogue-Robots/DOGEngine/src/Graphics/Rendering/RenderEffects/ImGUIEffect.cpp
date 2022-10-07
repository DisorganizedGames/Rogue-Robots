#include "ImGUIEffect.h"
#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ImGUIBackend.h"

namespace DOG::gfx
{
	ImGUIEffect::ImGUIEffect(GlobalEffectData& globalEffectData, RGBlackboard& blackboard, ImGUIBackend* backend) :
		RenderEffect(globalEffectData, blackboard),
		m_imgui(backend)
	{
	}

	void ImGUIEffect::Add(RenderGraph& rg)
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
				rd->Cmd_SetViewports(cmdl, m_globalEffectData.bbVP);
				rd->Cmd_SetScissorRects(cmdl, m_globalEffectData.bbScissor);
				m_imgui->Render(rd, cmdl);
			});
	}
}