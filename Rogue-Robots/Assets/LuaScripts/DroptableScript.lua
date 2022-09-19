
barrel_drop_table = {
	grenade = {1, "Grenade Barrel"},
	laser = {1, "Laser Barrel"},
	liquid = {1, "Liquid Barrel"}
}

magazine_drop_table = {
	fire = {1, "Fire Magazine"},
	frost = {1, "Frost Magazine"},
	elec = {1, "Electricity Magazine"}
}

misc_drop_table = {
	full_auto = {1, "Full Auto Switch"},
	burst = {1, "Burst Switch"},
	scatter = {1, "Scatter Shot"},
	charge_shot = {1, "Charge Shot"},
	radar = {1, "Radar"}
}

drops = {
	barrel_drop = {2, barrel_drop_table},
	magazine_drop = {1, magazine_drop_table},
	misc_drop = {1, misc_drop_table}
}

enemy_drops = {
	drop = {9, drops},
	nothing = {1, "Nothing"}
}

function GetDrop(dropItem)
	if type(dropItem) == "table" then
		return DropGuaranteed(dropItem)
	else
		return dropItem
	end
end

function TotalProb(dropTable) 
	tot = 0
	for k, v in pairs(dropTable) do
		tot = tot + (1/v[1])
	end
	return tot
end

function DropGuaranteed(table)
	acc = 0
	rnd = math.random()
	tot = TotalProb(table)

	for key, value in pairs(table) do
		if rnd < acc + ((1/value[1])/tot) then
			return GetDrop(value[2])
		end
		acc = acc + ((1/value[1])/tot)
	end

	return "Nothing" -- Should never occur
end

function Drop()
	sample_count = 100000
	samples = {}
	for i=0, sample_count do 
		drop = DropGuaranteed(enemy_drops)
		if samples[drop] then
			samples[drop] = samples[drop] + 1
		else
			samples[drop] = 0
		end
	end

	print("-------------------------------------------------")
	for k, v in pairs(samples) do
		print(k .. ": " .. v/sample_count * 100 .. "%")
	end
end