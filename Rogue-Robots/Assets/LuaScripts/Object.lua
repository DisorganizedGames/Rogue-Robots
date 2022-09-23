local ObjectManager = {}

local Object = {
	OnStart = nil,
	OnUpdate = nil
}

function ObjectManager:CreateObject(o)
	return Object:New(o)
end

function Object:New(o)
	o = o or {}
	setmetatable(o, self)
	self.__index = self
	return o
end

return ObjectManager