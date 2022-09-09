#pragma once
#include "LuaW.h"

class LuaContext
{
private:
	LuaW m_luaW;
public:
	LuaContext(LuaW luaW);
};

