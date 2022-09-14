#pragma once
#include "../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{	
	/*
		Temporarily caches file path to texture to avoid re-reading existing texture daata
	*/
	class RenderDevice;
	class GPUGarbageBin;
	class UploadContext;

	class TextureManager
	{
	public:
		struct TextureSubresource
		{
			std::span<u8> data;
			u32 width{ 0 };
			u32 height{ 0 };
		};

		// Uncompressed (assuming R8G8B8A8)
		struct MippedTexture2DSpecification
		{
			std::vector<TextureSubresource> dataPerMip;
			bool srgb{ false };
		};

	public:
		TextureManager(RenderDevice* rd, GPUGarbageBin* bin);

		Texture LoadTexture(const std::string& name, const MippedTexture2DSpecification& spec, UploadContext& ctx);
		bool Exists(const std::string& name);
		Texture GetTexture(const std::string& name);
		

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };

		std::unordered_map<std::string, Texture> m_textures;

	};

}