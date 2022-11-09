#pragma once

#include "../Graphics/RHI/ImGUIBackend.h"
#include "../ECS/EntityManager.h"
#include "../ECS/Component.h"

#include "Types/AssetTypes.h"
#include "DOGEngineTypes.h"
#include "RigAnimator.h"

namespace DOG
{
	// rip BoneJovi
	class AnimationManager
	{
	private:
		// scale, rot, trans
		static constexpr u8 N_KEYS = 3;
		static constexpr u8 MAX_CLIPS = 10;
	private:
		enum class KeyType
		{
			Scale = 0,
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
		void Test(f32 dt);
		void UpdateAnimationComponent(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt) const;
		void UpdateSkeleton(DOG::RigAnimator& animator, const u32 offset);
		void SetPlayerBaseStates();

		DirectX::FXMVECTOR GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick);

		// Gets the S/R/T keyframe data from active animation clips in animation component
		void ExtractClipNodeInfluences(const ClipData* pcData, const std::vector<AnimationData>& anims, const KeyType key, const u8 nClips, const u8 rigID, const u8 group);

		// RIG ANIMATOR
		void CalculateSRT(RigAnimator& ac, const u8 rigID);
		void ExtractClipNodeInfluences(RigAnimator& animator, const KeyType key, const u32 group, const u32 rigID);
	private:
		std::vector<ImportedRig*> m_rigs;
		std::array<RigAnimator, 4> m_playerRigAnimators;
		std::array<DirectX::XMVECTOR, MAX_CLIPS * NodeCount(MIXAMO_RIG_ID)> m_partialSRT{DirectX::XMVECTOR{}};
		std::array<DirectX::XMVECTOR, MAX_CLIPS * NodeCount(MIXAMO_RIG_ID)> m_fullbodySRT{DirectX::XMVECTOR{} };
	private:
		bool m_bonesLoaded = false;
		bool m_playerAnimatorsLoaded = false;
		static constexpr i32 ROOT_NODE = 0;
		static constexpr i32 ROOT_JOINT = 2;
		static constexpr DirectX::XMFLOAT3 m_baseScale = { 1.f, 1.f, 1.f };
		static constexpr DirectX::XMFLOAT4 m_baseRotation = { 0.f, 0.f, 0.f, 0.f };
		static constexpr DirectX::XMFLOAT3 m_baseTranslation = { 0.f, 0.f, 0.f };
		
		// IMGUI RELATED
	private:
		RigAnimator mRigAnimator;
		f32 m_imguiGroupWeightA = 0.0f;
		bool m_imguiApplyRootTranslation = true;
		DirectX::FXMMATRIX ImguiTransform(i32 joint);
		std::vector<DirectX::XMFLOAT3> m_imguiSca;
		std::vector<DirectX::XMFLOAT3> m_imguiRot;
		std::vector<DirectX::XMFLOAT3> m_imguiPos;
	public:
		void SpawnControlWindow(bool& open);
	};
}

