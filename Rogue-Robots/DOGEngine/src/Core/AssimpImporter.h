#pragma once
#include "DOGEngineTypes.h"

namespace DOG
{
	class AssimpImporter
	{
		enum class Shape
		{
			Prism,
		};
	public:
		AssimpImporter(const std::filesystem::path& path);
		AssimpImporter(const u32 shape);

		std::shared_ptr<ImportedModel> GetShape() const { return m_loadedModel; }
		std::shared_ptr<ImportedModel> GetResult() const { return m_loadedModel; }

	private:
		std::shared_ptr<ImportedModel> m_loadedModel;

	};
}