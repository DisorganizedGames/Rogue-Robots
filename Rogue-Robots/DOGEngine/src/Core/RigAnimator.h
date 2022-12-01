#pragma once
#include "../ECS/Component.h"
#include "Types/AssetTypes.h"

namespace DOG
{
	static constexpr u32 N_GROUPS = 3;
	static constexpr u32 MAX_CLIPS = 6;
	static constexpr u32 MAX_TARGETS = 3;
	static constexpr u8 LOOPING = 0;
	static constexpr u8 ACTION = 1;
	static constexpr i8 NO_ANIMATION = -1;
	static constexpr u16 BASE_PRIORITY = 0;

	struct RigSpecifics
	{
		u8 nJoints;
		u8 rootJoint;
		u8 headJoint;
		std::pair<u8, u8> groupMasks[3];
		u8 groupParent[3] = { 0 };
	};

	static constexpr u8 fullBodyGroup = 0;
	static constexpr u8 groupA = 1;
	static constexpr u8 groupB = 2;

	static constexpr u8 N_RIGS = 1;
	static constexpr u8 MIXAMO_RIG_ID = 0;
	//static constexpr u8 SCRPIO_RIG_ID = 1;
	static constexpr RigSpecifics RIG_SPECIFICS[N_RIGS]{ { 65, 4, 5, {std::make_pair<u8, u8>(0, 67), std::make_pair<u8, u8>(57, 10), std::make_pair<u8, u8>(3, 54) }, { 0, 0, 0 }} };
	static constexpr RigSpecifics MIXAMO_RIG = RIG_SPECIFICS[MIXAMO_RIG_ID];

	// true if idx within group joint span
	static constexpr bool InGroup(const u8 group, const u8 rigID, const u8 jointIdx) {
		return jointIdx >= RIG_SPECIFICS[rigID].groupMasks[group].first &&
			jointIdx < RIG_SPECIFICS[rigID].groupMasks[group].first + RIG_SPECIFICS[rigID].groupMasks[group].second;
	}
	// return group index of join
	static constexpr u8 GetGroup(const u8 rigID, const u8 idx) {
		return groupA * InGroup(rigID, idx, groupA) + groupB * InGroup(rigID, idx, groupA);
	};
	// return startJoint and number of joints that group influences
	static constexpr std::pair<u8, u8> GetNodeStartAndCount(const u8 rigID, const u8 group) {
		return RIG_SPECIFICS[rigID].groupMasks[group];
	};
	// return startJoint and number of joints that group influences
	static constexpr u8 NodeCount(const u8 rigID) {
		return RIG_SPECIFICS[rigID].groupMasks[fullBodyGroup].second;
	};
	// start and stop index of group
	static constexpr std::pair<u8, u8> GetNodeSpan(const u8 rigID, const u8 group) {
		return std::pair<u8, u8>{RIG_SPECIFICS[rigID].groupMasks[group].first,
			static_cast<u8>(RIG_SPECIFICS[rigID].groupMasks[group].first + RIG_SPECIFICS[rigID].groupMasks[group].second)};
	};
	
	// Data needed from Animation Clip for calculating pose influence, maybe add this as a secondary component on the entity for network syncing
	struct ClipData
	{
		i32 aID = -1;
		f32 weight = 0.f;
		f32 tick = 0.f;
	};
	struct RigAnimator
	{
		static constexpr f32 TARGET_LOOPING = 0.f;
		static constexpr f32 TARGET_ACTION = 1.f;
		ImportedRig* rigData = {};

		f32 globalTime = 0.f;
		using Setter = AnimationComponent::Setter;
		struct BlendSpecification
		{
			f32 durationLeft = 0.f;
			f32 playbackRate = 1.f;
			f32 transitionStart = 0.f;
			f32 transitionLength = 0.f;
			f32 startWeight = 0.f;
			f32 currentWeight = 0.f;
			f32 targetWeight = 0.f;
		};
		struct Clip
		{
			i32 id = -1;
			f32 normalizedTime = {};
			f32 timeScale = {};
			f32 duration = {};
			f32 totalTicks = {};

			f32 targetWeight = 1.0f;
			f32 currentWeight = 0.0f;
		};
		struct ClipSet
		{
			u32 nTargets = 0;
			u32 nTotalClips = 0;
			BlendSpecification blend = {};
			std::array<Clip, 6> clips;
		};
		struct Group
		{
			u32 priority = BASE_PRIORITY;
			i32 parent = -1;
			f32 weight = 0.f;
			BlendSpecification blend = {};
			ClipSet looping;
			ClipSet action;
		};
		std::array<Group, N_GROUPS> groups = {};
		std::array<BlendSpecification, N_GROUPS> groupBlends = {};
		std::array<u32, N_GROUPS> groupClipCount = { 0 };
		std::array<ClipData, N_GROUPS * 12> clipData = {};

		RigAnimator();

		void AddTargetSet(Setter& setter, ClipSet& set, u32 clipCount, bool looping);
		void AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime = 0.f);
		void PostAddFixUp(ClipSet& set, u32 idx);
		void ModifyClip(Clip& clip, Setter& setter, u32 setIdx);
		void UpdateClipCW(ClipSet& set, u32 idx, u32 newNrOfTargets);
		
		std::array<f32, 6> GetWeights(ClipSet& set);

		i32 GetClipIndex(const ClipSet& set, const i32 animationID);
		f32 LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue);
		f32 BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue);

		void SetReturnTransition(BlendSpecification& bs, f32 delta);

		void UpdateBsWeight(BlendSpecification& bs);

		void UpdateBsDuration(BlendSpecification& bs, f32 dt);

		void UpdateBS(BlendSpecification& bs, f32 dt);

		void UpdateSet(ClipSet& set, u32 clipIdx, f32 blendWeight, f32 dt, bool looping = false);

		u32 UpdateGroup(u32 groupIdx, u32 startClipIdx, f32 dt);

		void ResetGroup(Group& group);

		f32 ClipNormalizedTime(Clip& c, const f32 dt, bool loop);

		void ProcessAnimationComponent(AnimationComponent& ac);

		u32 PreProcessSetter(Setter& setter);

		void ProcessSetter(Setter& setter);
		void SetBlendSpecifications(Setter& setter);
		void TransitionOutGroupBS(Setter& setter, BlendSpecification& bs);

		void SetClipsBS(Setter& setter);
		void SetSetBS(Setter& setter);
		void SetGroupBS(Setter& setter);

		void ResetSetter(Setter& setter);

		f32 GetGroupWeight(u32 group);
		bool ParentHigherPrio(u32 group);

		u32 GetGroupStartIdx(u32 group);

		void Update(f32 dt);
	};
}