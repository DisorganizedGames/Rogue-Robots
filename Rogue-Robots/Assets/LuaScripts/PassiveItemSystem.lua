require("PassiveItems")

local originalPlayerStats = nil
local itemsDirty = true
passiveItems = {

}

function OnStart() 
	EventSystem:Register("ItemPickup" .. EntityID, OnPickup)

	-- Temporary until we have a more central place to keep it
	EventSystem:Register("BulletEnemyHit" .. EntityID, OnBulletHit)
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
	
	local hp = Entity:GetPlayerStat(EntityID, "health")

	for key, item in pairs(passiveItems) do
		stats = item[1]:affect(item[2], stats)
	end
	
	stats.health = hp
	Entity:SetPlayerStats(EntityID, stats)
	print(Entity:GetPlayerStat(EntityID, "maxHealth"))

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

function OnBulletHit(enemy)
	local health = Entity:GetPlayerStat(EntityID, "health")
	local maxHealth = Entity:GetPlayerStat(EntityID, "maxHealth")
	local lifeSteal = Entity:GetPlayerStat(EntityID, "lifeSteal")
	
	health = math.min(health + lifeSteal, maxHealth)

	Entity:SetPlayerStat(EntityID, "health", health)

	itemsDirty = true
end

function OnDestroy()
	EventSystem:UnRegister("ItemPickup" .. EntityID, OnPickup)

	-- Temporary until we have a more central place to keep it
	EventSystem:UnRegister("BulletEnemyHit" .. EntityID, OnBulletHit)
end