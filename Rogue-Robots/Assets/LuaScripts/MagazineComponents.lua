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
		frostMaterial = Render:CreateMaterial({x=0.0, y=0.0, z=1.0}, 0.0, 0.0),
		Update = function(self, bullet)
			Entity:AddComponent(bullet.entity, "FrostEffect", 10.0)
			Entity:AddComponent(bullet.entity, "SubMeshRender", self.frostMaterial)
		end
	}
end

--Add more components here.

return MagazineComponents