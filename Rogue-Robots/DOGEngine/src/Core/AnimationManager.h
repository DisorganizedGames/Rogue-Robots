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
		// scale, rot, trans
		static constexpr u32 N_KEYS = 3;
		using ClipData = DOG::RealAnimationComponent::ClipRigData;

		static constexpr u8 m_mixamoIdx = 0;
		static constexpr u8 m_enemyIdx = 1;
		static constexpr u8 m_mixamoTotalJoints = 65;
		static constexpr u8 m_mixamoTotalNodes = 67;
		static constexpr u8 m_mixamoGroupAStrtIdx = 57;	// LowerBody
		static constexpr u8 m_mixamoGroupAStopIdx = 67; // LowerBody
		static constexpr u8 m_mixamoGroupBStrtIdx = 2;	// UpperBody
		static constexpr u8 m_mixamoGroupBStopIdx = 57;	// UpperBody
		// tmp
		static constexpr u8 MAX_ANIMATIONS = 3;
		static constexpr i32 m_noAnimation = -1;
		static constexpr u8 m_animationNumeroUno = 0;
		static constexpr u8 m_animationNumeroDos = 1;
		static constexpr u8 m_maxClips = 4;
		static constexpr u8 m_nExtra = 4;
		static constexpr u8 m_extra[m_nExtra] = {1, 2, 3, 4};

		
		enum class KeyType
		{
			Scale = 0,
			Rotation,
			Translation,
		};
		static constexpr u8 groupA = 0;
		static constexpr u8 groupB = 1;
		struct RigSpecifics
		{
			u8 nJoints;
			u8 nNodes;
			u8 rootJoint;
			u8 fullbodyGroup;
			std::pair<u8, u8> groupMasks[2];
		};
		
		static constexpr u8 N_RIGS = 1;
		static constexpr u8 MIXAMO_RIG_ID = 0;
		static constexpr RigSpecifics RIG_SPECIFICS[N_RIGS]{ { 65, 67, 4, 2, {{57, 10}, {5, 52}}} };
		static constexpr RigSpecifics MIXAMO_RIG = RIG_SPECIFICS[0];
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
		void UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::RealAnimationComponent& animator);

		DirectX::FXMVECTOR GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick);
		DirectX::FXMVECTOR ExtractScaling(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR ExtractRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR ExtractWeightedAvgRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR ExtractRootTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);

		void CalculateSRT(const std::vector<AnimationData>& anims, const RealAnimationComponent& ac, const u8 rigID);
		// Gets the S/R/T keyframe data from active animation clips in animation component
		void ExtractClipNodeInfluences(const ClipData* pcData, const std::vector<AnimationData>& anims, const KeyType key, const u8 nClips, const u8 rigID, const u8 group);
	private:
		std::vector<ImportedRig*> m_rigs;
		std::array<DirectX::XMVECTOR, m_maxClips* MIXAMO_RIG.nNodes> m_partialMixamoSRT{ DirectX::XMVECTOR{} };
		std::array<DirectX::XMVECTOR, m_maxClips* MIXAMO_RIG.nNodes> m_fullbodyMixamoSRT{ DirectX::XMVECTOR{} };
	private:
		//test
		bool m_up3 = false;
		bool m_gogogo = false;
		bool oldFirstTime = true;
		bool newFirstTime = true;
		bool newNewFirstTime = true;
		DirectX::XMVECTOR m_oldDebugVec5[3];
		DirectX::XMVECTOR m_newDebugVec5[3];
		DirectX::XMVECTOR m_newNewDebugVec5[3];
		std::vector<DirectX::XMMATRIX> m_debugOldTF;
		std::vector<DirectX::XMMATRIX> m_debugNewTF;
		std::vector<DirectX::XMMATRIX> m_debugNewNewTF;
		static constexpr u32 nDebugClips = 3;
		f32 m_debugTime = 0.0f;
		struct LogClip
		{
			f32 weight = 0.0f;
			f32 normW = 0.0f;
			f32 localT = 0.f;
			f32 debugT = 0.0f;
			f32 normT = 0.0f;
			f32 cTick = 0.0f;
			f32 nTicks = 0.0f;
			LogClip(f32 t, f32 w, f32 nw, f32 nt, f32 ct, f32 tt)
				: debugT(t), weight(w), normW(nw), normT(nt), cTick(ct), nTicks(tt) {};
		};
		std::vector<std::vector<LogClip>> loggedClips;
		bool m_bonesLoaded = false;
		f32 m_imguiGroupWeightA = 0.0f;
		f32 m_imguiGroupWeightB = 0.0f;
		i32 m_imguiMaskSpan1Strt = 0;
		i32 m_imguiMaskSpan1Stop = 100;
		i32 m_imguiMaskSpan2Strt = 0;
		i32 m_imguiMaskSpan2Stop = 100;
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

