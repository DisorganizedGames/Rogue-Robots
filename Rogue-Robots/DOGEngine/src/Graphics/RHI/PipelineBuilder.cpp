#include "PipelineBuilder.h"

namespace DOG::gfx
{
    GraphicsPipelineBuilder::GraphicsPipelineBuilder()
    {
        RasterizerBuilder rasterizer{};
        BlendBuilder blend{};
        DepthStencilBuilder ds{};
        ds.SetDepthEnabled(false);        // Depth testing disabled by default

        SetRasterizer(rasterizer);
        SetBlend(blend);
        SetDepthStencil(ds);
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthFormat(DepthFormat format)
    {
        switch (format)
        {
        case DepthFormat::D32:
            m_desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;               // Fully qualififies the format: DXGI_FORMAT_R32_TYPELESS
            break;
        case DepthFormat::D32_S8:
            m_desc.dsvFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;    // Fully qualififies the format: DXGI_FORMAT_R32G8X24_TYPELESS
            break;
        case DepthFormat::D24_S8:
            m_desc.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;       // Fully qualififies the format: DXGI_FORMAT_R24G8_TYPELESS
            break;
        case DepthFormat::D16:
            m_desc.dsvFormat = DXGI_FORMAT_D16_UNORM;               // Fully qualififies the format: DXGI_FORMAT_R16_TYPELESS
            break;
        default:
            assert(false);
        }
        return *this;
    }

    GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetShader(const CompiledShader* shader)
    {
        switch (shader->shaderType)
        {
        case ShaderType::Vertex:
            m_desc.vs = shader;
            break;
        case ShaderType::Hull:
            m_desc.hs = shader;
            break;
        case ShaderType::Domain:
            m_desc.ds = shader;
            break;
        case ShaderType::Geometry:
            m_desc.gs = shader;
            break;
        case ShaderType::Pixel:
            m_desc.ps = shader;
            break;
        default:
            assert(false);
        }
        return *this;
    }
}