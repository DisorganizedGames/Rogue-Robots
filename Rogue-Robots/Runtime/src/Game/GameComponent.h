#pragma once
#include <DOGEngine.h>
#include "Scripting/ScriptManager.h"

using namespace DOG;

struct GunComponent : public Component<GunComponent>
{
	TempScript* gunScript;
};