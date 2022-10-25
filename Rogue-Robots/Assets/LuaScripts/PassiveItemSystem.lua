require("PassiveItems")

local originalPlayerStats = nil
local itemsDirty = true
passiveItems = {

}

function OnStart() 
	EventSystem:Register("ItemPickup" .. EntityID, OnPickup)
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

function OnPickup(pickup)
	if Entity:HasComponent(pickup, "PassiveItem") then
		local type = Entity:GetPassiveType(pickup)
		if passiveItems[type] then
			passiveItems[type][2] = passiveItems[type][2] + 1
		else
			passiveItems[type] = { passiveItemsMap[type], 1 }
		end
		itemsDirty = true
	end
end

