#include "../../../Core/Types/MeshTypes.h"
#include "../../../Core/Types/MaterialTypes.h"


namespace DOG::gfx
{
	struct StaticModel
	{
		MeshContainer mesh;
		std::vector<MaterialHandle> mats;
	};
}