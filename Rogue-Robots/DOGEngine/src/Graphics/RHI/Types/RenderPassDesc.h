#pragma once
#include "ResourceDescs.h"

namespace DOG::gfx
{
	enum class RenderPassBeginAccessType
	{
		Discard,		// Indicates that the targets associated CAN be discarded
		Preserve,		// Indicates that the associated targets HAVE to be preserved
		Clear			// Indicates that the associated targets HAVE to be cleared
	};

	enum class RenderPassEndingAccessType
	{
		Discard,
		Preserve,
		Resolve			// Not supported for now
	};

	// Combined Begin/End
	enum class RenderPassAccessType
	{
		Discard_Discard,
		Discard_Preserve,

		Preserve_Discard,
		Preserve_Preserve,

		Clear_Discard,
		Clear_Preserve
	};

	static std::pair<RenderPassBeginAccessType, RenderPassEndingAccessType> GetAccessTypes(RenderPassAccessType access)
	{
		switch (access)
		{
		case RenderPassAccessType::Discard_Discard:
			return { RenderPassBeginAccessType::Discard, RenderPassEndingAccessType::Discard };
		case RenderPassAccessType::Discard_Preserve:
			return { RenderPassBeginAccessType::Discard, RenderPassEndingAccessType::Preserve };
		case RenderPassAccessType::Preserve_Discard:
			return { RenderPassBeginAccessType::Preserve, RenderPassEndingAccessType::Discard };
		case RenderPassAccessType::Preserve_Preserve:
			return { RenderPassBeginAccessType::Preserve, RenderPassEndingAccessType::Preserve };
		case RenderPassAccessType::Clear_Discard:
			return { RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Discard };
		case RenderPassAccessType::Clear_Preserve:
			return { RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Preserve };

		default:
			assert(false);
			return {};
		}
	}

	struct RenderPassTargetDesc
	{
		TextureView view;
		RenderPassBeginAccessType beginAccess;
		RenderPassEndingAccessType endAccess;

		// Resolve not supported yet
	};

	struct RenderPassDepthStencilTargetDesc
	{
		TextureView view;
		RenderPassBeginAccessType depthBeginAccess;
		RenderPassEndingAccessType depthEndAccess;
		RenderPassBeginAccessType stencilBeginAccess;
		RenderPassEndingAccessType stencilEndAccess;
	};

	enum class RenderPassFlag : u8
	{
		None,
		AllowUnorderedAccessWrites,
	};

	struct RenderPassDesc
	{
		std::vector<RenderPassTargetDesc> renderTargetDescs{};
		std::optional<RenderPassDepthStencilTargetDesc> depthStencilDesc{};
		RenderPassFlag flags{ RenderPassFlag::None };
	};

	class RenderPassBuilder
	{
	public:

		// Ordering matters
		RenderPassBuilder& AppendRT(
			TextureView view,
			RenderPassBeginAccessType beginAccess,
			RenderPassEndingAccessType endingAccess)
		{
			RenderPassTargetDesc desc{};
			desc.view = view;
			desc.beginAccess = beginAccess;
			desc.endAccess = endingAccess;
			desc.view = view;
			m_rpDesc.renderTargetDescs.push_back(desc);
			return *this;
		}

		// Conveniece API if the user only cares about using depth
		RenderPassBuilder& AddDepth(
			TextureView view,
			RenderPassBeginAccessType depthBegin,
			RenderPassEndingAccessType depthEnd)
		{
			AddDepthStencil(view, depthBegin, depthEnd, RenderPassBeginAccessType::Discard, RenderPassEndingAccessType::Discard);
			return *this;
		}

		RenderPassBuilder& AddDepthStencil(
			TextureView view,
			RenderPassBeginAccessType depthBegin,
			RenderPassEndingAccessType depthEnd,
			RenderPassBeginAccessType stencilBegin,
			RenderPassEndingAccessType stencilEnd)
		{
			m_rpDesc.depthStencilDesc = RenderPassDepthStencilTargetDesc{};
			m_rpDesc.depthStencilDesc->view = view;
			m_rpDesc.depthStencilDesc->depthBeginAccess = depthBegin;
			m_rpDesc.depthStencilDesc->depthEndAccess = depthEnd;
			m_rpDesc.depthStencilDesc->stencilBeginAccess = stencilBegin;
			m_rpDesc.depthStencilDesc->stencilEndAccess = stencilEnd;

			return *this;
		}

		const RenderPassDesc& Build() { return m_rpDesc; }

	private:
		RenderPassDesc m_rpDesc{};
	};
}