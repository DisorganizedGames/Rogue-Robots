#pragma once

namespace DOG::gfx
{
	class RenderGraph;
	class ImGUIBackend;

	class ImGUIPass
	{
	public:
		ImGUIPass(ImGUIBackend* backend);
		
		void AddPass(RenderGraph& rg);

	private:
		ImGUIBackend* m_imgui{ nullptr };
	};
}