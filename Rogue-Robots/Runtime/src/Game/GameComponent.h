#pragma once
#include <DOGEngine.h>
#include "Scripting/ScriptManager.h"

struct GunComponent : public DOG::Component<GunComponent>
{
	TempScript* gunScript;
};