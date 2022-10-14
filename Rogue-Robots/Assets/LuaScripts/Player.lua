require("VectorMath")

--function OnStart()
--	EventSystem:Register("PlayerDeath" .. EntityID, PlayerDie) --Is called if there is no barrelcomponent.
--end

function OnUpdate()
	pStats = Entity:GetPlayerStats(EntityID)
	if pStats.health <= 0.0 then
		PlayerDies()
		pStats.health = 100.0
		Entity:ModifyComponent(EntityID, "PlayerStats", pStats)
	end
end

function PlayerDies()
	print("Aaargh...!!!")
	Entity:ModifyComponent(EntityID, "Transform", Vector3.New(25, 25, 25), 1)
end