#pragma once
#include "DOGEngineTypes.h"

namespace DOG
{
	class ShapeCreator
	{
	public:
		ShapeCreator(const Shape shape, u32 tessFactor1, u32 tessFactor2 = 3);
		void MakeSphere(u32 latDiv, u32 longDiv);
		void MakeSheet(u32 xDiv, u32 zDiv);
		void MakePrism(u32 longDiv);
		void MakeCone(u32 longDiv);
		std::shared_ptr<ImportedModel> GetResult() const { return m_loadedModel; }
	private:
		std::shared_ptr<ImportedModel> m_loadedModel;
	};
}