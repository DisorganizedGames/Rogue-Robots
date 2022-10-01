#pragma once

#include "../Graphics/RHI/ImGUIBackend.h"
#include "../ECS/EntityManager.h"
#include "../ECS/Component.h"

#include "Types/AssetTypes.h"
#include "DOGEngineTypes.h"

namespace DOG
{
	// rip BoneJovi
	class AnimationManager
	{
	private:
		static constexpr u8 MAX_ANIMATIONS = 2;
		enum class KeyType
		{
			Scale,
			Rotation,
			Translation,
		};
	public:
		AnimationManager();
		~AnimationManager();
		void UpdateJoints();
		// Matrices to upload to Vertex Shader
		std::vector<DirectX::XMFLOAT4X4> m_vsJoints;
	private:
		void UpdateAnimationComponent(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt) const;
		void UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::AnimationComponent& animator);

		DirectX::FXMVECTOR GetAnimationComponent(const std::vector<DOG::AnimationKey>& keys, KeyType component, f32 tick);
		DirectX::FXMMATRIX CalculateNodeTransformation(const DOG::AnimationData&, i32 nodeID, f32 animTick);
		DirectX::FXMMATRIX CalculateBlendTransformation(i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
	private:
		bool m_bonesLoaded = false;
		// IMGUI RELATED
		i32 m_imguiProfilePerformUpdate = 1;
		bool m_imguiRootTranslation = false;
		i32 m_imguiSelectedBone = 1;
		std::vector<DirectX::XMFLOAT3> m_imguiSca;
		std::vector<DirectX::XMFLOAT3> m_imguiRot;
		std::vector<DirectX::XMFLOAT3> m_imguiPos;
	public:
		void SpawnControlWindow(bool& open);
	};
}

