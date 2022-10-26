passiveItemsMap = {}
function passiveItemsMap:Register(name, item) 
	self[name] = item
end

local passiveItemTemplate = {
	-- Local values. These should not change, unless one wants them to change globally
	maxHealthBoost = 10,	

	affect = function(self, stackCount, stats)
		local newStats = stats

		-- Change the stats, like increasing the max health
		newStats.maxHealth = newStats.maxHealth + self.maxHealthBoost * stackCount

		return newStats
	end
}
passiveItemsMap:Register("Template", passiveItemTemplate)


-- Actually just a copy of the template item
-- Passive item that increases the max health of the player that picked it up by "boost"
local maxHealthBoost = {
	boost = 10,	

	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.maxHealth = newStats.maxHealth + self.boost * stackCount
		return newStats
	end
}
passiveItemsMap:Register("MaxHealthBoost", maxHealthBoost)

-- Passive item that boosts the movement speed of the player that picked it up by "boost"
local speedBoost = {
	boost = 2,

	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.speed = newStats.speed + self.boost * stackCount
		return newStats
	end
}

-- Passive item that increases the lifesteal on bullet hit for the player that picked it up by "stealAmount"
local lifeSteal = {
	stealAmount = 1,

	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.lifeSteal = newStats.lifeSteal + self.stealAmount * stackCount
		return newStats
	end
}

