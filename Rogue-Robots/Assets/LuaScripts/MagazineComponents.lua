local MagazineComponents = {}

--[[
ARGUMENTS
1:
]]

function MagazineComponents:BasicEffect()
	return
	{
		Update = function(self)
		end,

		GetECSType = function(self)
			return 0
		end,
	}
end

function MagazineComponents:FrostEffect()
	return
	{
		--frostMaterial = Render:CreateMaterial({x=0.188, y=0.835, z=0.784}, 0.0, 0.0, { 0.0, 0.0, 0.0 }),
		Update = function(self, bullet, playerEntityID)
			Entity:AddComponent(bullet.entity, "FrostEffect", 4.0)
			Entity:AddComponent(bullet.entity, "SubMeshRender", MaterialPrefabs:GetMaterial("FrostMaterial"))
		end,

		GetECSType = function(self)
			return 1
		end,
	}
end

function MagazineComponents:FireEffect()
	return
	{
		Update = function(self, bullet, playerEntityID)
			Entity:AddComponent(bullet.entity, "FireEffect", playerEntityID, 4.0, 50.0)
			Entity:AddComponent(bullet.entity, "SubMeshRender", MaterialPrefabs:GetMaterial("FireMaterial"))
		end,

		GetECSType = function(self)
			return 2
		end,
	}
end

--Add more components here.

return MagazineComponents