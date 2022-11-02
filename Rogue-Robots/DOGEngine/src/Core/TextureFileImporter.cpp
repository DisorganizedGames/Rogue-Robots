#include "TextureFileImporter.h"
#include "Compressonator/compressonator.h"

namespace DOG
{
	bool TextureFileImporter::s_initialized = false;

	void TextureFileImporter::Initialize()
	{
		if (!s_initialized)
		{
			CMP_InitFramework();
			s_initialized = true;
		}
	}


	TextureFileImporter::TextureFileImporter(const std::filesystem::path& path, bool genMips)
	{
		assert(s_initialized);

		// Result is empty if path is empty
		if (path.empty())
			return;

		std::lock_guard<std::mutex> lock(m_mutex);

		CMP_MipSet mipSetIn{};
		memset(&mipSetIn, 0, sizeof(CMP_MipSet));
		auto cmpStatus = CMP_LoadTexture(path.string().c_str(), &mipSetIn);
		if (cmpStatus != CMP_OK)
		{
			std::printf("Error %d: Loading source file!\n", cmpStatus);
			assert(false);
		}

		m_result = std::make_shared<ImportedTextureFile>();


		CMP_INT mipRequests = 1;
		if (genMips)
		{
			// Arbitrarily request 3 mips for now
			mipRequests = 3;

			mipRequests = (std::min)(mipRequests, mipSetIn.m_nMaxMipLevels);
			if (mipSetIn.m_nMipLevels <= 1)
			{
				//------------------------------------------------------------------------
				// Checks what the minimum image size will be for the requested mip levels
				// if the request is too large, a adjusted minimum size will be returns
				//------------------------------------------------------------------------
				CMP_INT n_min_size = CMP_CalcMinMipSize(mipSetIn.m_nHeight, mipSetIn.m_nWidth, mipRequests);

				//--------------------------------------------------------------
				// now that the minimum size is known, generate the miplevels
				// users can set any requested minumum size to use. The correct
				// miplevels will be set acordingly.
				//--------------------------------------------------------------
				CMP_GenerateMIPLevels(&mipSetIn, n_min_size);
			}


		}

		// Grab mips
		for (i32 i = 0; i < mipRequests; ++i)
		{
			CMP_MipLevel* mip{ nullptr };
			CMP_GetMipLevel(&mip, &mipSetIn, i, 0);

			std::vector<u8> data;
			data.resize(mip->m_dwLinearSize);
			std::memcpy(data.data(), mip->m_pbData, mip->m_dwLinearSize);

			ImportedTextureFileMip mipData{};
			mipData.data = std::move(data);
			mipData.width = mip->m_nWidth;
			mipData.height = mip->m_nHeight;

			m_result->dataPerMip.push_back(std::move(mipData));
		}

		CMP_FreeMipSet(&mipSetIn);
	}
}