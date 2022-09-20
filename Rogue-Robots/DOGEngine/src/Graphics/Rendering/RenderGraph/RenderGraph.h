#pragma once

namespace DOG::gfx
{
	class RenderDevice;

	class RenderPassResources;
	class RenderPassBuilder;

	class RenderGraph
	{
	private:
		struct RenderPass
		{

		};

	public:
		
		template <typename PassData>
		void AddPass(const std::string& name,
			const PassData& passData,
			const std::function<void(RenderPassBuilder&, PassData&)>& buildFunc,
			const std::function<void(RenderDevice*, const RenderPassResources& resources, const PassData& passData)> execFunc)
		{
			// Build directly


		}

	private:

	};


}