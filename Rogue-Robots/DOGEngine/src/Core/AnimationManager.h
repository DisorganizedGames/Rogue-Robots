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
		// tmp
		static constexpr u8 MAX_ANIMATIONS = 3;
		static constexpr i32 m_noAnimation = -1;
		static constexpr u8 m_animationNumeroUno = 0;
		static constexpr u8 m_animationNumeroDos = 1;
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

		void UpdateMovementAnimation(DOG::AnimationComponent& ac, const f32 dt);
		void UpdateMovementAnimation2(DOG::AnimationComponent& ac, const f32 dt);

		DirectX::FXMVECTOR GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick);
		DirectX::FXMVECTOR CalculateScaling(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR CalculateRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR CalculateRotation2(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR CalculateTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);

		void UpdateClips(DOG::AnimationComponent& ac, const f32 dt);
		void UpdateBezier(AnimationComponent::AnimationClip& clip);
		void UpdateLinear(AnimationComponent::AnimationClip& clip);
		void UpdateLinearGT(AnimationComponent::AnimationClip& clip, const f32 globalTime);
		std::vector<ImportedRig*> m_rigs;
	private:
		bool m_bonesLoaded = false;
		i32 m_imguiMinMaskIdx = 100;
		i32 m_imguiMaxMaskIdx = 0;
		static constexpr i32 m_rootNodeIdx = 0;
		static constexpr i32 m_rootBoneIdx = 2;
		static constexpr DirectX::XMFLOAT3 m_baseScale = { 1.f, 1.f, 1.f };
		static constexpr DirectX::XMFLOAT4 m_baseRotation = { 0.f, 0.f, 0.f, 0.f };
		static constexpr DirectX::XMFLOAT3 m_baseTranslation = { 0.f, 0.f, 0.f };
		// tmp for testing different blend types.
		static constexpr i32 m_modeImguiBlend = 0;
		static constexpr i32 m_modeTransitionLinearBlend = 1;
		static constexpr i32 m_modeTransitionBezierBlend = 2;
		// IMGUI RELATED
		DirectX::FXMMATRIX ImguiTransform(i32 joint);

		bool m_imguiResetPos = false;
		bool m_imguiTestMovement = true;
		f32 m_imguiMovementSpeed = 0.0f;
		f32 m_imguiMovementAngle = 0.0f;
		bool m_imguiPause = false;
		bool m_imguiMatching = false;
		i32 m_imguiProfilePerformUpdate = 1;
		bool m_imguiRootTranslation = false;
		i32 m_imguiSelectedBone = 1;
		std::vector<DirectX::XMFLOAT3> m_imguiSca;
		std::vector<DirectX::XMFLOAT3> m_imguiRot;
		std::vector<DirectX::XMFLOAT3> m_imguiPos;

		static constexpr f32 m_imguiBlendMin = 0.0f;
		static constexpr f32 m_imguiBlendMax = 1.0f;
		static constexpr f32 m_imguiTransitionMin = 0.0f;
		static constexpr f32 m_imguiTransitionMax = 1.0f;
		static constexpr f32 m_imguiTimeScaleMin = -2.0f;
		static constexpr f32 m_imguiTimeScaleMax = 2.0f;
		static constexpr i32 m_imguiNumUpdatesMin = 0;
		static constexpr i32 m_imguiNumUpdatesMax = 100;
		static constexpr f32 m_imguiJointRotMin = -180.0f;
		static constexpr f32 m_imguiJointRotMax = 180.0f;
		static constexpr f32 m_imguiJointScaMin = -10.0f;
		static constexpr f32 m_imguiJointScaMax = 10.0f;
		static constexpr f32 m_imguiJointPosMin = -1.0f;
		static constexpr f32 m_imguiJointPosMax = 1.0f;
		static constexpr f32 m_imguiNormalizedTimeMin = 0.0f;
		static constexpr f32 m_imguiNormalizedTimeMax = 1.0f;
	public:
		void SpawnControlWindow(bool& open);

	};
}

