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

local iter = 0

function OnUpdate()
	if not itemsDirty then
		return
	end

	-- Happens first time to store Lua copy
	if not originalPlayerStats then
		originalPlayerStats = Entity:GetPlayerStats(EntityID)
	end

	-- Keep grabbing latest stats, for health, since that is dynamic
	-- The rest of the stats can use the old system --> Using OriginalPlayerStats and stacking it every frame..
	local currPlayerStats = Entity:GetPlayerStats(EntityID)
	originalPlayerStats.health = currPlayerStats["health"]

	-- Stores local copy to be updated
	local stats = {}
	for k, v in pairs(originalPlayerStats) do
		stats[k] = v
		print(k, v)
	end

	-- Use effects to affect stats (this happens every frame!)
	for key, item in pairs(passiveItems) do
		stats = item[1]:affect(item[2], stats)
	end

	Entity:SetPlayerStats(EntityID, stats)

	itemsDirty = false

end

function OnPickup(pickup)
	local pickupTypeString = Entity:GetEntityTypeAsString(pickup)

	if pickupTypeString == "MaxHealthBoost" or pickupTypeString == "SpeedBoost" or pickupTypeString == "SpeedBoost2" or pickupTypeString == "JumpBoost" then
		if passiveItems[pickupTypeString] then
			passiveItems[pickupTypeString][2] = passiveItems[pickupTypeString][2] + 1
		else
			passiveItems[pickupTypeString] = { passiveItemsMap[pickupTypeString], 1 }
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