
-- List of currently active temporary pickups
tempPickups = {}

-- Template to use when implementing a permanent effect pickup item.
templatePickup = {
	affect = function(self, stats)
		-- Do something to the player stats, like increasing health
		-- Entity:ModifyComponent(Player:GetID(), "PlayerStats", stats)
	end
}

-- Template to use when implementing a temporary effect pickup item.
templateTempPickup = {
	affect = function(self, stats)
		-- Do something to the player stats, like increasing health
		-- Entity:ModifyComponent(Player:GetID(), "PlayerStats", stats)
		
		table.insert(tempPickups, {
			co = couroutine.create(function(params)
					local beginWait = os.clock()
					repeat co.yield() until os.clock() - beginWait > 3.0 -- Duration in seconds
					-- Reset stats
				end
			), 
			params = {self, stats}}
		)

	end
}

function OnUpdate()
	for index, pu in ipairs(tempPickups) do
		if not coroutine.resume(pu.co, pu.params) then
			table.remove(tempPickups, index)
		end
	end
end

function OnPickup(item)
	item:affect(playerStats)
end

EventSystem:Register("OnItemPickup", OnPickup)
