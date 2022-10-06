function OnStart()
	--print("LuaTest", LuaTest)
	--LuaTest.SetNumber(20)
	--LuaTest.m_number = 50
	--LuaTest.SetNumber(100)
	--LuaTest:SetNumber(20)
end

function OnCollisionEnter(self, entity1, entity2)
	print("OnEntity1", entity1)
	print("OnEntity2", entity2)
end