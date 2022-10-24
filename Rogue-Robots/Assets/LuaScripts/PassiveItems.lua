passiveItemsMap = {}
function passiveItemsMap:Register(name, item) 
	self[name] = item
end

local passiveItemTemplate = {
	-- Local values
	maxHealthBoost = 10,	

	affect = function(self, stackCount, stats)
		local newStats = stats

		-- Change the stats, like increasing the max health
		newStats.maxHealth = newStats.maxHealth + self.maxHealthBoost * stackCount

		return newStats
	end
}
passiveItemsMap:Register("Template", passiveItemTemplate)
