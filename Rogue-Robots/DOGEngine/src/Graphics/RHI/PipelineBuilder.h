#pragma once
#include "Types/PipelineTypes.h"
#include "Types/DepthTypes.h"

namespace DOG::gfx
{
	class RasterizerBuilder
	{
	public:
		RasterizerBuilder()
		{
			m_desc.FillMode = D3D12_FILL_MODE_SOLID;
			m_desc.CullMode = D3D12_CULL_MODE_BACK;
			m_desc.FrontCounterClockwise = false;
			m_desc.DepthBias = 0;
			m_desc.DepthBiasClamp = m_desc.SlopeScaledDepthBias = 0.f;
			m_desc.DepthClipEnable = true;
			m_desc.MultisampleEnable = m_desc.AntialiasedLineEnable = false;
			m_desc.ForcedSampleCount = 0;
			m_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

		RasterizerBuilder& SetFillMode(D3D12_FILL_MODE mode) { m_desc.FillMode = mode; return *this; };
		RasterizerBuilder& SetCullMode(D3D12_CULL_MODE mode) { m_desc.CullMode = mode; return *this; };
		RasterizerBuilder& SetFrontIsCCW(bool enabled) { m_desc.FrontCounterClockwise = enabled; return *this; };

		RasterizerBuilder& SetDepthClipEnabled(bool enabled) { m_desc.DepthClipEnable = enabled; return *this; };
		RasterizerBuilder& SetMultisampleEnabled(bool enabled) { m_desc.MultisampleEnable = enabled; return *this; };

		RasterizerBuilder& SetDepthBias(i32 bias) { m_desc.DepthBias = bias; return *this; };
		RasterizerBuilder& SetDepthBiasClamp(float clamp) { m_desc.DepthBiasClamp = clamp; return *this; };
		RasterizerBuilder& SetSlopeScaledDepthBias(float bias) { m_desc.SlopeScaledDepthBias = bias; return *this; };
		RasterizerBuilder& SetAALineEnabled(bool enabled) { m_desc.AntialiasedLineEnable = enabled; return *this; };
		RasterizerBuilder& SetForcedSampleCount(u32 count) { m_desc.ForcedSampleCount = count; return *this; };
		RasterizerBuilder& SetConservativeRasterEnabled(bool mode)
		{
			m_desc.ConservativeRaster = mode ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			return *this;
		};

		operator D3D12_RASTERIZER_DESC() const { return m_desc; }

	private:
		D3D12_RASTERIZER_DESC m_desc{};
	};

	class BlendBuilder
	{
	public:
		BlendBuilder()
		{
			m_desc.AlphaToCoverageEnable = false;
			m_desc.IndependentBlendEnable = false;

			for (u8 i = 0; i < 8; ++i)
			{
				// defaults
				m_desc.RenderTarget[0].BlendEnable = false;
				m_desc.RenderTarget[0].LogicOpEnable = false;

				m_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
				m_desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
				m_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD
					;
				m_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
				m_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
				m_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

				m_desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
				m_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			}
		}

		BlendBuilder& SetAlphaToCoverageEnabled(bool enabled) { m_desc.AlphaToCoverageEnable = enabled; return *this; };
		BlendBuilder& SetIndependentBlend(bool enabled = false) { m_desc.IndependentBlendEnable = enabled; return *this; };

		// Ordering matters
		BlendBuilder& AppendRTBlend(const D3D12_RENDER_TARGET_BLEND_DESC& desc)
		{
			m_desc.RenderTarget[m_blendsAppended++] = desc;
			return *this;
		};

		operator D3D12_BLEND_DESC() const { return m_desc; }

	private:
		u8 m_blendsAppended{ 0 };
		D3D12_BLEND_DESC m_desc{};
	};

	class DepthStencilBuilder
	{
	public:
		DepthStencilBuilder()
		{
			// Set defaults
			m_desc.DepthEnable = false;		// ---- Depth disabled by default!
			m_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			// Reverse Z by default!
			m_desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			m_desc.StencilEnable = false;
			m_desc.StencilReadMask = 0xFF;
			m_desc.StencilWriteMask = 0xFF;

			m_desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			m_desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			m_desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			m_desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			m_desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			m_desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			m_desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			m_desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		}

		DepthStencilBuilder& SetDepthEnabled(bool enabled) { m_desc.DepthEnable = enabled; return *this; }
		DepthStencilBuilder& SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask) { assert(m_desc.DepthEnable); m_desc.DepthWriteMask = mask; return *this; }
		DepthStencilBuilder& SetDepthFunc(D3D12_COMPARISON_FUNC func) { assert(m_desc.DepthEnable); m_desc.DepthFunc = func; return *this; }

		DepthStencilBuilder& SetStencilEnabled(bool enabled) { m_desc.StencilEnable = enabled; return *this; }
		DepthStencilBuilder& SetStencilReadMask(u8 mask) { assert(m_desc.StencilEnable); m_desc.StencilReadMask = mask; return *this; }
		DepthStencilBuilder& SetStencilWriteMask(u8 mask) { assert(m_desc.StencilEnable); m_desc.StencilWriteMask = mask; return *this; }

		DepthStencilBuilder& SetStencilOpFrontface(const D3D12_DEPTH_STENCILOP_DESC& desc) { m_desc.FrontFace = desc; return *this; }
		DepthStencilBuilder& SetStencilOpBackface(const D3D12_DEPTH_STENCILOP_DESC& desc) { m_desc.BackFace = desc; return *this; }

		operator D3D12_DEPTH_STENCIL_DESC() const { return m_desc; }

	private:
		D3D12_DEPTH_STENCIL_DESC m_desc{};
	};

	class GraphicsPipelineBuilder
	{
	public:
		GraphicsPipelineBuilder();

		GraphicsPipelineBuilder& SetTopology(PrimitiveTopology topology) { m_desc.topology = topology; return *this; }
		GraphicsPipelineBuilder& SetSampleMask(u32 mask) { m_desc.sampleMask = mask; return *this; }
		GraphicsPipelineBuilder& SetMultisample(u8 count, u8 quality) { m_desc.sampleCount = count; m_desc.sampleQuality = quality; return *this; };
		GraphicsPipelineBuilder& SetDepthFormat(DepthFormat format);

		GraphicsPipelineBuilder& SetShader(const CompiledShader* shader);

		// Ordering matters
		GraphicsPipelineBuilder& AppendRTFormat(DXGI_FORMAT format) { m_desc.rtvFormats[m_desc.numRenderTargets++] = format; return *this; }

		GraphicsPipelineBuilder& SetRasterizer(RasterizerBuilder& builder) { m_desc.rasterizer = builder; return *this; }
		GraphicsPipelineBuilder& SetBlend(BlendBuilder& builder) { m_desc.blendState = builder; return *this; }
		GraphicsPipelineBuilder& SetDepthStencil(DepthStencilBuilder& builder) { m_desc.depthStencil = builder; return *this; }

		GraphicsPipelineBuilder& SetNumControlPatches(u32 num) { m_desc.numControlPatches = num; return *this; }

		const GraphicsPipelineDesc& Build() const { return m_desc; }

	private:
		GraphicsPipelineDesc m_desc;
	};

}