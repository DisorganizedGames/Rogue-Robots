require("PassiveItems")

local originalPlayerStats = nil
local itemsDirty = true
passiveItems = {
	Template = {passiveItemsMap["Template"], 1}
}

function OnStart() 
	EventSystem:Register("PassiveItemPickup" .. EntityID, function() itemsDirty = true end)
end

function OnUpdate()
	if not itemsDirty then
		return
	end

	if not originalPlayerStats then
		originalPlayerStats = Entity:GetPlayerStats(EntityID)
	end
	
	local stats = {}
	for k, v in pairs(originalPlayerStats) do
		stats[k] = v
	end

	for key, item in pairs(passiveItems) do
		stats = item[1]:affect(item[2], stats)
	end
	
	print(EntityID .. " Health: " .. stats.maxHealth)

	Entity:ModifyComponent(EntityID, "PlayerStats",  stats)

	itemsDirty = false
end

function OnCollisionEnter(self, e1, e2)
	-- Physics makes this not work, but the code is left here for future reference
	if Entity:HasComponent(e2, "PassiveItem") then
		local pType = Entity:GetPassiveType(e2)

		if passiveItems[pType] then
			passiveItems[pType][2] = passiveItems[pType][2] + 1
		else
			passiveItems[pType] = { passiveItemsMap[pType], 1 }
		end

		Entity:DestroyEntity(e2)
		EventSystem:InvokeEvent("PassiveItemPickup" .. EntityID)
	end
end

