#pragma once
#include "../../RHI/RHITypes.h"
#include "../../RHI/Types/ResourceDescs.h"

namespace DOG::gfx
{
	class MeshTable;
	class MaterialTable;

	/*
		Don't bloat this structure unless you are sure that it is something that belongs in global scope.
		Use the RGBlackboard for specific interpass communication.
	*/
	struct GlobalPassData
	{
		// VP/Scissor which follows render resolution
		Viewports defRenderVPs;
		ScissorRects defRenderScissors;

		// VP/Scissor which follows backbuffer resolution
		Viewports bbVP;
		ScissorRects bbScissor;

		u32 globalDataDescriptor{ UINT_MAX };
		const u32* perFrameTableOffset{ nullptr };

		const MeshTable* meshTable{ nullptr };

	};
}