#pragma once
#include "DOGEngineTypes.h"

namespace DOG
{
	class ShapeCreator
	{
	public:
		ShapeCreator(const Shape shape, u32 tessFactor1, u32 tessFactor2 = 3);
		void MakeSphere(const u32 latDiv, const u32 longDiv);
		void MakeSheet(const u32 xDiv, const u32 zDiv);
		void MakePrism(const u32 longDiv);
		void MakeCone(const u32 longDiv);
		std::shared_ptr<ImportedModel> GetResult() const { return m_loadedModel; }
	private:
		std::shared_ptr<ImportedModel> m_loadedModel;
	};
}