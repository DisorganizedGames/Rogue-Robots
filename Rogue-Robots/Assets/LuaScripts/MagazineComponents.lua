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
		frostEffectSound = Asset:LoadAudio("Assets/Audio/GunSounds/Frost.wav"),

		Update = function(self, bullet, playerEntityID, magazineAudioEntity)
			Entity:AddComponent(bullet.entity, "FrostEffect", 4.0)
			Entity:AddComponent(bullet.entity, "SubMeshRender", MaterialPrefabs:GetMaterial("FrostMaterial"))
			Entity:PlayAudio(magazineAudioEntity, self.frostEffectSound, true)
		end,

		GetECSType = function(self)
			return 1
		end,
	}
end

function MagazineComponents:FireEffect()
	return
	{
		fireEffectSound = Asset:LoadAudio("Assets/Audio/GunSounds/FlameWhoosh6.wav"),

		Update = function(self, bullet, playerEntityID, magazineAudioEntity)
			Entity:AddComponent(bullet.entity, "FireEffect", playerEntityID, 4.0, 50.0)
			Entity:AddComponent(bullet.entity, "SubMeshRender", MaterialPrefabs:GetMaterial("FireMaterial"))
			Entity:PlayAudio(magazineAudioEntity, self.fireEffectSound, true)
		end,

		GetECSType = function(self)
			return 2
		end,
	}
end

--Add more components here.

return MagazineComponents