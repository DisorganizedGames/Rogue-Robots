local MagazineComponents = {}

--[[
ARGUMENTS
1:
]]

function MagazineComponents:BasicEffect()
	return
	{
		Update = function(self)
		end
	}
end

function MagazineComponents:FireEffect()
	return
	{
		Update = function(self)
		end
	}
end

function MagazineComponents:FrostEffect()
	return
	{
		frostMaterial = Render:CreateMaterial({x=0.188, y=0.835, z=0.784}, 0.0, 0.0),
		Update = function(self, bullet)
			Entity:AddComponent(bullet.entity, "FrostEffect", 4.0)
			Entity:AddComponent(bullet.entity, "SubMeshRender", self.frostMaterial)
		end
	}
end

--Add more components here.

return MagazineComponents