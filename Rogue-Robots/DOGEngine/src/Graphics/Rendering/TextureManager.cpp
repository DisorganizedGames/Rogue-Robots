#include "TextureManager.h"
#include "../RHI/RenderDevice.h"
#include "UploadContext.h"
#include "GPUGarbageBin.h"

namespace DOG::gfx
{
    TextureManager::TextureManager(RenderDevice* rd, GPUGarbageBin* bin) :
        m_rd(rd),
        m_bin(bin)
    {
    }

    Texture TextureManager::LoadTexture(const std::string& name, const MippedTexture2DSpecification& spec, UploadContext& ctx)
    {
        // Returns existing if any
        auto it = m_textures.find(name);
        if (it != m_textures.cend())
            return it->second;

        // Assuming 32-bit per pixel and SRGB for now (needs to be changed when using Block Compressed textures)
        const DXGI_FORMAT texFormat = spec.srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        const u8 bytesPerPixel = sizeof(u32);
        // D3D12 alignment requirement for textures
        static constexpr u32 TEXALIGNMENT = 512;

        const u32 baseWidth = spec.dataPerMip[0].width;
        const u32 baseHeight = spec.dataPerMip[0].height;

        assert(baseWidth != 0 && baseHeight != 0);

        // Create texture
        auto d = TextureDesc(
            MemoryType::Default, texFormat,
            baseWidth, baseHeight, 1)
            .SetMipLevels((u32)spec.dataPerMip.size());
        Texture tex = m_rd->CreateTexture(d);

        // Load each mip to texture
        for (u32 mip = 0; mip < spec.dataPerMip.size(); ++mip)
        {
            const auto& mipData = spec.dataPerMip[mip];

            // Get aligned row pitch for destination
            const u32 rowPitchOriginal = mipData.width * bytesPerPixel;
            const u32 rowPitchAligned = (1 + ((rowPitchOriginal - 1) / TEXALIGNMENT)) * TEXALIGNMENT;

            ctx.PushUploadToTexture(
                // Target
                tex, mip, { 0, 0, 0 },
                // Source
                (void*)mipData.data.data(),
                texFormat, mipData.width, mipData.height, 1,
                rowPitchAligned
            );
        }

        m_textures[name] = tex;
        
        return tex;
    }
    bool TextureManager::Exists(const std::string& name)
    {
        return m_textures.contains(name);
    }
    Texture TextureManager::GetTexture(const std::string& name)
    {
        return m_textures.find(name)->second;
    }
}