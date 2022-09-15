#pragma once
#include "Types/AssetTypes.h"

namespace DOG
{
	class TextureFileImporter
	{
	private:
		static bool s_initialized;
	public:
		TextureFileImporter(const std::filesystem::path& path, bool genMips);

		std::shared_ptr<ImportedTextureFile> GetResult() const { return m_result; }
		
	private:
		std::shared_ptr<ImportedTextureFile> m_result;
	};
}