#pragma once
#include "../ECS/Component.h"
#include "Types/AssetTypes.h"

namespace DOG
{
	static constexpr u32 N_GROUPS = 3;
	static constexpr u32 MAX_CLIPS = 10;
	static constexpr u32 MAX_TARGETS = 3;
	static constexpr u8 LOOPING = 0;
	static constexpr u8 ACTION = 1;

	struct Clip
	{
		i32 aID = -1;
		bool loop = false;

		f32 normalizedTime = {};	// Tick
		f32 timeScale = {};			// Tick
		f32 duration = {};			// Tick
		f32 totalTicks = {};		// Tick

		f32 transitionStart = {};
		f32 transitionLength = 0.0f;// Weight
		f32 startWeight = 0.0f;		// Weight
		f32 targetWeight = 1.0f;	// Weight
		f32 currentWeight = 0.0f;	// Weight
	};
	struct ClipData
	{
		i32 aID = -1;
		f32 weight = 0.f;
		f32 tick = 0.f;
	};
	struct BlendSpecification
	{
		f32 scale = 0.f;
		f32 duration = 0.f;
		f32 transitionStart = 0.f;
		f32 transitionLength = 0.f;
		f32 startWeight = 1.f;
	};
	struct ClipSet
	{
		using Setter = AnimationComponent::Setter2;
		f32 weight = 0.f;
		f32 playbackRate = 0.f;
		u32 startClip = 0;
		u32 nTargetClips = 0;
		u32 nTotalClips = 0;
	};
	struct AnimationGroup
	{
		u32 primary = 0;
		f32 weight = {};
		BlendSpecification blend;
		std::array<ClipSet, 2> sets;
	};
	struct RigAnimator
	{
		using Setter = AnimationComponent::Setter2;
		
		f32 globalTime = 0.0f;
		ImportedRig* rigData = {};

		std::array<Clip, 50> clips = {};
		// index for First clip of corresponding group
		std::array<u32, N_GROUPS> groupClipCount = { 0 };
		std::array<ClipData, 50> clipData = {};
		std::array<AnimationGroup, N_GROUPS> groups = {};

		RigAnimator();

		void Update(const f32 dt);

		i32 GetClipIndex(const ClipSet& set, const i32 animationID);
		void AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime);
		void ModifyClip(Clip& clip, Setter& setter, u32 setIdx);
		// Set new target animation set
		void AddTargetSet(ClipSet& set, Setter& setter, u32 clipCount);

		u32 GetGroupStartIdx(u32 group);

		u32 UpdateGroup(AnimationGroup& group, const u32 clipIdx, const f32 dt);

		u32 UpdateClipSet(ClipSet& set, const u32 clipIdx, const f32 dt, bool looping = false);

		f32 ClipNormalizedTime(Clip& c, const f32 delta, bool loop);
		f32 ClipTick(const Clip& c, const f32 nt);

		f32 BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue);

		f32 LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue);

		void HandleAnimationComponent(AnimationComponent& ac);

		void HandleSetter(Setter& setter, AnimationGroup& group);

		std::pair<f32, f32> ClipTickWeight(Clip& c, f32 dt);

		void SetClip(Setter& setter, u32 setIdx, Clip& clip);

		void SetBlendSpec(AnimationGroup& group, const Setter& setter);

		void UpdateBlendSpec(AnimationGroup& group, const f32 dt);

		void NormalizeWeights(ClipSet& set);

		void TransitionOutClips(ClipSet& set, const f32 transitionStart, const f32 transitionLen);

		u32 ValidateSetter(Setter& setter);

		void ResetSetter(Setter& setter);
	};
}

//
//struct AnimationComponent
//{
//	static constexpr u8 MAX_SETTERS = 10;
//	static constexpr u8 MAX_TARGET_ANIMS = 3;
//	u32 offset;
//	i8 rigID = 0;
//	i8 animatorID = -1;
//	i8 addedSetters = 0;
//	struct Setter
//	{
//		bool loop;
//		bool desired;
//
//		u8 animationID;
//		u8 group;
//		f32 transitionLength;
//		f32 playbackRate;
//	};
//	struct TargetAnimations
//	{
//		f32 transitionLength;
//		BlendMode blendMode;
//		u8 group;
//		u8 flags;
//		u8 priority;
//		bool loop;
//		u8 animationIDs[MAX_TARGET_ANIMS];
//		f32 targetWeights[MAX_TARGET_ANIMS];
//		f32 playbackRates[MAX_TARGET_ANIMS];
//	};
//	std::array<TargetAnimations, MAX_SETTERS> groupSetters;
//	std::array<Setter, MAX_SETTERS> animSetters;
//};