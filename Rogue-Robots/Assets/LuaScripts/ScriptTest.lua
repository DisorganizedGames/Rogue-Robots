function OnStart()
	--print("LuaTest", LuaTest)
	--LuaTest.SetNumber(20)
	--LuaTest.m_number = 50
	--LuaTest.SetNumber(100)
	--LuaTest:SetNumber(20)
end

function OnUpdate()
	print("Hello")
	coroutine.yield()
	print("Y")
end

function OnCollisionEnter(self, entity1, entity2)
	print("OnCollisionEnter POGGERS")
	print("OnEntity1", entity1)
	print("OnEntity2", entity2)
end

function OnCollisionExit(self, entity1, entity2)
	print("OnCollisionExit ALSO POGGERS")
	print("OnEntity1", entity1)
	print("OnEntity2", entity2)
end