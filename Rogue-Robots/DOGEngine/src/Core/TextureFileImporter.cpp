#include "TextureFileImporter.h"
#include "Compressonator/compressonator.h"

#include <DirectXTex/DirectXTex.h>
#include "../Graphics/RHI/DX12/D11Device.h"

namespace DOG
{
	bool TextureFileImporter::s_initialized = false;

	void TextureFileImporter::Initialize()
	{
		if (!s_initialized)
		{
			CMP_InitFramework();

			// Code kept here in case initialize order is changed
			// There is another CoInitializeEx which happens earlier
			//HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			//assert(SUCCEEDED(hr));

			s_initialized = true;

		}
	}


	TextureFileImporter::TextureFileImporter(const std::filesystem::path& path, bool genMips, bool srgb)
	{
		UNREFERENCED_PARAMETER(genMips);

		HRESULT hr{ S_OK };
		assert(s_initialized);

		std::lock_guard<std::mutex> lock(m_mutex);

		// Result is empty if path is empty
		if (path.empty())
			return;

		auto newPath = path;
		newPath.replace_extension("dds");
		
		// Sanity check, KEEP THIS
		//if (srgb)
		//	std::cout << "Is SRGB!\n";
		//else
		//	std::cout << "Is not SRGB!\n";

		// Generate DDS if it doesnt exist
		if (!std::filesystem::exists(newPath))
		{
			DirectX::TexMetadata md;
			DirectX::ScratchImage img;
			hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, &md, img);
			assert(SUCCEEDED(hr));

			//DirectX::ScratchImage mipChain;
			//hr = DirectX::GenerateMipMaps(img.GetImages(), img.GetImageCount(), md, DirectX::TEX_FILTER_DEFAULT, md.mipLevels, mipChain);
			//assert(SUCCEEDED(hr));

			DirectX::TEX_COMPRESS_FLAGS compressFlags = DirectX::TEX_COMPRESS_BC7_QUICK;

			DXGI_FORMAT format = DXGI_FORMAT_BC7_UNORM;
			if (srgb)
			{
				compressFlags |= DirectX::TEX_COMPRESS_SRGB_IN;
				compressFlags |= DirectX::TEX_COMPRESS_SRGB_OUT;
				format = DXGI_FORMAT_BC7_UNORM_SRGB;
			}

			DirectX::ScratchImage bcImg;
			hr = DirectX::Compress(GetD11Device(), *img.GetImages(), format, compressFlags, DirectX::TEX_THRESHOLD_DEFAULT, bcImg);
			assert(SUCCEEDED(hr));

			D11Flush();

			hr = DirectX::SaveToDDSFile(bcImg.GetImages(), bcImg.GetImageCount(), bcImg.GetMetadata(), DirectX::DDS_FLAGS_NONE, newPath.c_str());
			assert(SUCCEEDED(hr));
		}


		// Load new
		DirectX::TexMetadata md;
		auto image = std::make_unique<DirectX::ScratchImage>();
		hr = DirectX::LoadFromDDSFile(newPath.c_str(), DirectX::DDS_FLAGS_NONE, &md, *image);
		assert(SUCCEEDED(hr));

		m_result = std::make_shared<ImportedTextureFile>();
		m_result->format = md.format;

		// Grab mips (2D texture support only right now)
		for (i32 i = 0; i < md.mipLevels; ++i)
		{
			auto mipData = image->GetImage(i, 0, 0);
			auto mipSize = mipData->slicePitch;

			std::vector<u8> data;
			data.resize(mipSize);
			std::memcpy(data.data(), mipData->pixels, mipSize);

			ImportedTextureFileMip mipFinal{};
			mipFinal.data = std::move(data);

			mipFinal.width = (u32)mipData->width;
			mipFinal.height = (u32)mipData->height;

			m_result->dataPerMip.push_back(std::move(mipFinal));
		}

		return;


		/*
			
			Compressonator code below kept to have a fallback ready in case Block Compression solution breaks
			for any reason.
		
		*/

		//// Load normal textures

		//std::lock_guard<std::mutex> lock(m_mutex);

		//CMP_MipSet mipSetIn;
		//memset(&mipSetIn, 0, sizeof(CMP_MipSet));
		//auto cmpStatus = CMP_LoadTexture(path.string().c_str(), &mipSetIn);
		//if (cmpStatus != CMP_OK)
		//{
		//	std::printf("Error %d: Loading source file!\n", cmpStatus);
		//	assert(false);
		//}

		//m_result = std::make_shared<ImportedTextureFile>();

		//bool isDDS = path.extension() == ".dds";
		//if (isDDS)
		//	genMips = false;

		//CMP_INT mipRequests = 1;
		//if (genMips)
		//{
		//	// Arbitrarily request 3 mips for now
		//	mipRequests = 3;

		//	mipRequests = (std::min)(mipRequests, mipSetIn.m_nMaxMipLevels);
		//	if (mipSetIn.m_nMipLevels <= 1)
		//	{
		//		//------------------------------------------------------------------------
		//		// Checks what the minimum image size will be for the requested mip levels
		//		// if the request is too large, a adjusted minimum size will be returns
		//		//------------------------------------------------------------------------
		//		CMP_INT n_min_size = CMP_CalcMinMipSize(mipSetIn.m_nHeight, mipSetIn.m_nWidth, mipRequests);

		//		//--------------------------------------------------------------
		//		// now that the minimum size is known, generate the miplevels
		//		// users can set any requested minumum size to use. The correct
		//		// miplevels will be set acordingly.
		//		//--------------------------------------------------------------
		//		CMP_GenerateMIPLevels(&mipSetIn, n_min_size);
		//	}


		//}



		//mipRequests = isDDS ? mipSetIn.m_nMaxMipLevels : mipRequests;
		//for (i32 i = 0; i < mipRequests; ++i)
		//{
		//	CMP_MipLevel* mip{ nullptr };
		//	CMP_GetMipLevel(&mip, &mipSetIn, i, 0);

		//	std::vector<u8> data;
		//	data.resize(mip->m_dwLinearSize);
		//	std::memcpy(data.data(), mip->m_pbData, mip->m_dwLinearSize);

		//	ImportedTextureFileMip mipData{};
		//	mipData.data = std::move(data);
		//	mipData.width = mip->m_nWidth;
		//	mipData.height = mip->m_nHeight;

		//	m_result->dataPerMip.push_back(std::move(mipData));
		//}

		//CMP_FreeMipSet(&mipSetIn);
	}
}