#include "../../Core/Types/AssetTypes.h"
#include "MeshTable.h"
#include "MaterialTable.h"

namespace DOG::gfx
{
	class RenderDevice;
	class UploadContext;

	class GraphicsBuilder
	{
		// Argument types
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
		GraphicsBuilder(
			RenderDevice* rd,
			UploadContext* dataUpCtx,
			UploadContext* texUpCtx,
			MeshTable* meshes,
			MaterialTable* mats);

		// Mesh + Materials per submesh
		StaticModel LoadCustomModel(
			const MeshTable::MeshSpecification& meshSpec, 
			const std::vector<MaterialTable::MaterialSpecification>& matSpecs);

		// Expose fundamentals
		Texture LoadTexture(const MippedTexture2DSpecification& spec);
		TextureView CreateTextureView(Texture tex, std::optional<TextureViewDesc> desc);


	private:
		RenderDevice* m_rd{ nullptr };

		MeshTable* m_meshTable{ nullptr };
		MaterialTable* m_matTable{ nullptr };

		UploadContext* m_uploadCtx{ nullptr };		// For general data 
		UploadContext* m_texUploadCtx{ nullptr };	// For textures (larger footprint)
	};
}