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
		static constexpr u8 m_extra = 5;
		enum class KeyType
		{
			Scale = 0,
			Rotation,
			Translation,
		};
		struct RigSpecifics
		{
			static constexpr u8 groupA = 0;
			static constexpr u8 groupB = 1;
			u8 nJoints;
			u8 nNodes;
			u8 nPartialGroups;
			u8 fullbodyGroup;
			u8 groupAmaskStrt;
			u8 groupAmaskStop;
			u8 groupBmaskStrt;
			u8 groupBmaskStop;
		};
		std::pair<u8, u8> GetNodeStartAndCount(const struct RigSpecifics rs, const u8 group) const
		{
			if (group == rs.groupA)
				return { rs.groupAmaskStrt, (u8)(rs.groupAmaskStop - rs.groupAmaskStrt + m_extra) };
			else if (group == rs.groupB)
				return { rs.groupBmaskStrt, (u8)(rs.groupBmaskStop - rs.groupBmaskStrt) };
			else
				return { (u8)0, rs.nNodes };
		}
		static constexpr u8 m_nRigs = 1;
		static constexpr u8 m_mixamoRigId = 0;
		//static constexpr RigSpecifics m_enemy1RigSpecs = { 65, 67, 3, 57, 67, 2, 57, 0 };
		static constexpr RigSpecifics m_rigSpecifics[m_nRigs]{ { 65, 67, 2, 2, 57, 67, 4, 57 } };
	public:
		AnimationManager();
		~AnimationManager();
		void UpdateJoints();
		// Matrices to upload to Vertex Shader
		std::vector<DirectX::XMFLOAT4X4> m_vsJoints;
	private:
		void UpdateAnimationComponent(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt) const;
		void UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::AnimationComponent& animator);
		void UpdateSkeleton3(const DOG::ImportedRig& rig, const DOG::RealAnimationComponent& animator);

		DirectX::FXMVECTOR GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick);
		DirectX::FXMVECTOR ExtractScaling(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR ExtractRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR ExtractWeightedAvgRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);
		DirectX::FXMVECTOR ExtractRootTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac);


		void CalculateSRT(const std::vector<AnimationData>& anims, const RealAnimationComponent& ac, const u8 rigID);
		// Gets the S/R/T keyframe data from active animation clips in animation component
		void ExtractClipNodeInfluences(const ClipData* pcData, const std::vector<AnimationData>& anims, const KeyType key, const u8 nClips, const u8 rigID, const u8 group);
		// Weighted average sum of the animation clip influences
		void SumNodeInfluences(std::vector<DirectX::XMVECTOR>& keyValues, const ClipData* cData, const KeyType key, const u8 nClips, const u8 startNode, const u8 nNodes, bool fullbody = false);

		void SetGroupInfluence(std::vector<DirectX::XMVECTOR>& srtVector, const std::vector<AnimationData>& anims, const RigSpecifics& rigSpec, const RealAnimationComponent& ac);
		void SetGroupInfluence(std::vector<DirectX::XMVECTOR>& srtVector, const u8 group, const std::vector<AnimationData>& anims, const RigSpecifics& rigSpec, const RealAnimationComponent& ac);

		void SetClipNodeInfluence(std::vector<DirectX::XMVECTOR>& tfv, const std::vector<AnimationData>& anims, const KeyType key, const RigSpecifics& rigSpec, const RealAnimationComponent& ac);

		void UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::RealAnimationComponent& animator);

		void SetScaling(std::vector<DirectX::XMVECTOR>& finalScaling, const u8 rigID, const RealAnimationComponent& ac);
		void SetGroupScalingInfluence(std::vector<DirectX::XMVECTOR>& scaling, const u8 group, const ImportedRig& rig, const RigSpecifics& rigSpec, const RealAnimationComponent& ac);

		void SetTranslation(std::vector<DirectX::XMVECTOR>& finalTranslation, const u8 rigID, const RealAnimationComponent& ac);
		void SetGroupTranslationInfluence(std::vector<DirectX::XMVECTOR>& translation, const u8 group, const ImportedRig& rig, const RigSpecifics& rigSpec, const RealAnimationComponent& ac);

		void SetRotation(std::vector<DirectX::XMVECTOR>& finalRotation, const u8 rigID, const RealAnimationComponent& ac);
		void SetGroupRotationInfluence(std::vector<DirectX::XMVECTOR>& rotation, const u8 group, const ImportedRig& rig, const RigSpecifics& rigSpec, const RealAnimationComponent& ac);

		std::vector<DirectX::FXMVECTOR> ExtractPose(const RigSpecifics& rigSpecs, const std::vector<AnimationData>& animations, const RealAnimationComponent& ac);

	private:
		// Animation Component functions
		void UpdateClips(DOG::AnimationComponent& ac, const f32 dt);
		void UpdateBezier(AnimationComponent::AnimationClip& clip);
		void UpdateLinear(AnimationComponent::AnimationClip& clip);
		void UpdateLinearGT(AnimationComponent::AnimationClip& clip, const f32 globalTime);
		f32 GetGroupWeight(RealAnimationComponent& ac, u8 group)
		{
			return ac.groupWeights[group];
		};
		std::vector<ImportedRig*> m_rigs;
	private:
		//test
		std::array<DirectX::XMVECTOR, m_maxClips * m_rigSpecifics[m_mixamoIdx].nNodes> m_partialMixamoSRT { DirectX::XMVECTOR{} };
		std::array<DirectX::XMVECTOR, m_maxClips * m_rigSpecifics[m_mixamoIdx].nNodes> m_fullbodyMixamoSRT{ DirectX::XMVECTOR{} };
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

