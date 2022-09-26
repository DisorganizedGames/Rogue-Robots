playerStats = {
	maxHealth = 100.0,
	health = 100.0,
	speed = 10.0,
	-- ...
}

tempPickups = {}

-- Template to use when implementing a permanent effect pickup item.
templatePickup = {
	affect = function(self, stats) 
		-- Do something to the player stats, like increasing health
	end
}

-- Template to use when implementing a temporary effect pickup item.
templatePickup = {
	affect = function(self, stats) 
		-- Do something to the player stats, like increasing health
		
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
