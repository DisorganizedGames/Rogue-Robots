function OnStart()
	print("LuaTest", LuaTest)
	LuaTest.SetNumber(20)
	LuaTest.m_number = 50
	LuaTest.SetNumber(100)
	--LuaTest:SetNumber(20)
end