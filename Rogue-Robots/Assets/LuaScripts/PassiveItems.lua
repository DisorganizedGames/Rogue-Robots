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


local currMaxHealthStackCount = 0

-- Actually just a copy of the template item
-- Passive item that increases the max health of the player that picked it up by "boost"
local maxHealthBoost = {
	boost = 10,	

	affect = function(self, stackCount, stats)
		local newStats = stats

		newStats.maxHealth = newStats.maxHealth + self.boost * stackCount

		-- This acts as the "OnPickup for passive items"
		-- If any stack change has been attained --> Up the HP
		-- No way to decrease stack, so when it does happen to decrease, game session has likely restarted
		local shouldIncreaseHP = stackCount ~= currMaxHealthStackCount
		if (stackCount == 0) then
			shouldIncreaseHP = false
		end
		if (shouldIncreaseHP) then
			newStats.health = stats.health + self.boost
		end

		currMaxHealthStackCount = stackCount
		return newStats
	end
}
passiveItemsMap:Register("MaxHealthBoost", maxHealthBoost)

-- Passive item that boosts the movement speed of the player that picked it up by "boost"
local speedBoost = {
	boost = 0.5,

	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.speed = newStats.speed + self.boost * stackCount
		return newStats
	end
}
passiveItemsMap:Register("SpeedBoost", speedBoost)

-- Passive item that boosts the movement speed of the player that picked it up by "boost"
local speedBoost2 = {
	boost = 1,

	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.speed = newStats.speed + self.boost * stackCount
		return newStats
	end
}
passiveItemsMap:Register("SpeedBoost2", speedBoost2)

local jumpBoost = {
	boost = 1,

	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.jumpSpeed = newStats.jumpSpeed + self.boost * stackCount
		return newStats
	end
}
passiveItemsMap:Register("JumpBoost", jumpBoost)

-- Passive item that increases the lifesteal on bullet hit for the player that picked it up by "stealAmount"
local lifeSteal = {
	stealAmount = .2,
	affect = function(self, stackCount, stats)
		local newStats = stats
		newStats.lifeSteal = newStats.lifeSteal + self.stealAmount * stackCount
		print(newStats.lifeSteal)
		return newStats
	end
}
passiveItemsMap:Register("LifeSteal", lifeSteal)

