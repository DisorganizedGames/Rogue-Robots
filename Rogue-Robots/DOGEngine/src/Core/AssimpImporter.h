#pragma once
#include "DOGEngineTypes.h"

namespace DOG
{
	class AssimpImporter
	{
	public:
		AssimpImporter(const std::filesystem::path& path);

		std::shared_ptr<ImportedModel> get_result() const { return m_loadedModel; }

	private:
		std::shared_ptr<ImportedModel> m_loadedModel;

	};
}