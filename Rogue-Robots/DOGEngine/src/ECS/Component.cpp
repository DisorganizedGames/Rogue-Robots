#include "Component.h"
namespace DOG
{
	static u32 componentID{ 0u };

	const u32 ComponentBase::GetID() noexcept
	{
		return componentID++;
	}

}