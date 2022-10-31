#pragma once

#include "../Graphics/RHI/ImGUIBackend.h"
#include "../ECS/EntityManager.h"
#include "../ECS/Component.h"

#include "Types/AssetTypes.h"
#include "DOGEngineTypes.h"
#include "Animator.h"

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
		using ClipData = DOG::Animator::ClipRigData;
		enum class KeyType
		{
			Scale = 0,
			Rotation,
			Translation,
		};
		
		//static constexpr RigSpecifics SCORPIO_RIG = RIG_SPECIFICS[1];

		static constexpr bool InGroup(const u8 group, const u8 rigID, const u8 idx) {
			return idx >= RIG_SPECIFICS[rigID].groupMasks[group].first &&
				idx < RIG_SPECIFICS[rigID].groupMasks[group].first + RIG_SPECIFICS[rigID].groupMasks[group].second;
		}
		static constexpr u8 GetGroup(const u8 rigID, const u8 idx){
			return groupA * InGroup(rigID, idx, groupA) + groupB * InGroup(rigID, idx, groupA);
		};
		static constexpr std::pair<u8, u8> GetNodeStartAndCount(const u8 rigID, const u8 group) {
			return group == RIG_SPECIFICS[rigID].fullbodyGroup ?
				std::pair<u8, u8>{ (u8)0, RIG_SPECIFICS[rigID].nNodes } : RIG_SPECIFICS[rigID].groupMasks[group];
		};
	public:
		AnimationManager();
		~AnimationManager();
		void UpdateJoints();
		// Matrices to upload to Vertex Shader
		std::vector<DirectX::XMFLOAT4X4> m_vsJoints;
	private:
		void UpdateAnimationComponent(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt) const;
		void UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::Animator& animator);

		DirectX::FXMVECTOR GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick);
		DirectX::FXMVECTOR ExtractScaling(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::Animator& ac);
		DirectX::FXMVECTOR ExtractRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::Animator& ac);
		DirectX::FXMVECTOR ExtractWeightedAvgRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::Animator& ac);
		DirectX::FXMVECTOR ExtractRootTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::Animator& ac);

		void CalculateSRT(const std::vector<AnimationData>& anims, const Animator& ac, const u8 rigID);
		// Gets the S/R/T keyframe data from active animation clips in animation component
		void ExtractClipNodeInfluences(const ClipData* pcData, const std::vector<AnimationData>& anims, const KeyType key, const u8 nClips, const u8 rigID, const u8 group);

		i8 GetNextAnimatorID()
		{
			static i8 id = 0;
			return id++;
		}
	private:
		std::vector<ImportedRig*> m_rigs;
		std::array<Animator, 4> m_playerAnimators;
		std::array<DirectX::XMVECTOR, MAX_CLIPS * MIXAMO_RIG.nNodes> m_partialSRT{ DirectX::XMVECTOR{} };
		std::array<DirectX::XMVECTOR, MAX_CLIPS * MIXAMO_RIG.nNodes> m_fullbodySRT{ DirectX::XMVECTOR{} };
	private:
		bool m_bonesLoaded = false;
		static constexpr i32 ROOT_NODE = 0;
		static constexpr i32 ROOT_JOINT = 2;
		static constexpr DirectX::XMFLOAT3 m_baseScale = { 1.f, 1.f, 1.f };
		static constexpr DirectX::XMFLOAT4 m_baseRotation = { 0.f, 0.f, 0.f, 0.f };
		static constexpr DirectX::XMFLOAT3 m_baseTranslation = { 0.f, 0.f, 0.f };
		
		// IMGUI RELATED
	private:
		std::pair<i32, i32> hackerman{ 0 , 0 };
		u8 hack[4] = { 0, 0, 0, 0 };
		f32 m_imguiGroupWeightA = 0.0f;
		bool m_imguiApplyRootTranslation = false;
		DirectX::FXMMATRIX ImguiTransform(i32 joint);
		std::vector<DirectX::XMFLOAT3> m_imguiSca;
		std::vector<DirectX::XMFLOAT3> m_imguiRot;
		std::vector<DirectX::XMFLOAT3> m_imguiPos;
	public:
		void SpawnControlWindow(bool& open);
	};
}

