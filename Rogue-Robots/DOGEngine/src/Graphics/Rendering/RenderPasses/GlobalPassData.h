#pragma once
#include "../../RHI/RHITypes.h"
#include "../../RHI/Types/ResourceDescs.h"

namespace DOG::gfx
{
	struct GlobalPassData
	{
		// VP/Scissor which follows render resolution
		Viewports defRenderVPs;
		ScissorRects defRenderScissors;

		// VP/Scissor which follows backbuffer resolution
		Viewports bbVP;
		ScissorRects bbScissor;
	};
}