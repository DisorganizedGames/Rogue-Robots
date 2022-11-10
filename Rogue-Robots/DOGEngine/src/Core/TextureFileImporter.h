#pragma once
#include "Types/AssetTypes.h"
#include <mutex>

#include <d3d11on12.h>

namespace DOG
{
	class TextureFileImporter
	{
	private:
		static bool s_initialized;
	public:
		static void Initialize();

		TextureFileImporter(const std::filesystem::path& path, bool genMips, bool srgb = false);

		std::shared_ptr<ImportedTextureFile> GetResult() const { return m_result; }

	private:
		std::shared_ptr<ImportedTextureFile> m_result;
		std::mutex m_mutex;
	};
}