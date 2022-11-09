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
	static constexpr i8 NO_ANIMATION = -1;

	// Animation clip, contains data for updating tick/weight of tick
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
	// Data needed from Animation Clip for calculating pose influence, maybe add this as a secondary component on the entity for network syncing
	struct ClipData
	{
		i32 aID = -1;
		f32 weight = 0.f;
		f32 tick = 0.f;
	};
	// Data for blending between groups as well as action vs. looping clips in same group
	struct BlendSpecification
	{
		f32 durationLeft = 0.f;
		f32 transitionStart = 0.f;
		f32 transitionLength = 0.f;
		f32 startWeight = 1.f;
		f32 targetValue = 1.f;
	};
	// Set of active clips,
	struct ClipSet
	{
		f32 weight = 0.f;
		f32 playbackRate = 0.f;
		u8 startClip = 0;
		u8 nTargetClips = 0;
		u8 nTotalClips = 0;
	};
	// Group of clips influencing rig, contains a 'looping' clipset and a 'action' clipset, and a blendspecification between them
	struct AnimationGroup
	{
		i16 parent = -1;
		u16 priority = 0;
		f32 weight = 1.f;
		BlendSpecification blend;
		std::array<ClipSet, 2> sets;
	};
	// Animator of a rigged entity, handles adding clips via RigAnimationComponent and updating/removing active clips
	struct RigAnimator
	{
		using Setter = AnimationComponent::Setter;
		
		bool baseStateSet = false;

		f32 globalTime = 0.0f;
		// Playback rate affecting all clips
		f32 playBackRate = 1.f;
		ImportedRig* rigData = {};

		// Snipp, Snipp here be Clips 
		std::array<Clip, 50> clips = {};

		// Clip data needed to update rig
		std::array<ClipData, 50> clipData = {};

		// Weights per group, fullbody 
		f32 gWeights[N_GROUPS - 1] = { 0.5f, 0.f };

		// Number of Clips influencing group
		std::array<u32, N_GROUPS> groupClipCount = { 0 };

		// Animation groups
		std::array<AnimationGroup, N_GROUPS> groups = {};
		std::array<BlendSpecification, N_GROUPS> groupBlends = {};

		RigAnimator();

		// Update active clips
		void Update(const f32 dt);

		// Set basestate of animatior
		void SetBaseState(ImportedRig* rigData);

		// Get index of clip
		i32 GetClipIndex(const ClipSet& set, const i32 animationID);

		// Add a new clip
		void AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime);

		// Modifies an active clip
		void ModifyClip(Clip& clip, Setter& setter, u32 setIdx);

		// Set new target animation set
		void AddTargetSet(ClipSet& set, Setter& setter, u32 clipCount);

		f32 GetGroupWeight(u32 group);

		// Get index of first clip from group
		u32 GetGroupStartIdx(u32 group);

		// Updates the blend spec and clips in group
		u32 UpdateGroup(u32 groupIdx, const u32 clipIdx, const f32 dt);

		// Updates clips in set
		u32 UpdateClipSet(ClipSet& set, const u32 clipIdx, const f32 dt, bool looping = false);

		// Update and return normalized time of a clip
		f32 ClipNormalizedTime(Clip& c, const f32 delta, bool loop);

		// Get current tick of a clip
		f32 ClipTick(const Clip& c, const f32 nt);

		// Blend functions
		f32 BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue);
		f32 LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue = 0.f);

		// Process the AnimationComponent and the Setters within, adding/modifying active clips that influences the entity
		void ProcessAnimationComponent(AnimationComponent& ac);
		void ProcessSetter(Setter& setter, u32 group);

		// Set clip data based on setter
		void SetClip(Setter& setter, u32 setIdx, Clip& clip);

		// Update blend spec returning current weight
		f32 UpdateBlendSpecification(BlendSpecification& bs, const f32 dt, const f32 currentValue = 0.f);

		// Set blendSpec that will return to previous state
		void SetReturningBlendSpec(BlendSpecification& bs, const f32 transitionLen, const f32 target, const f32 duration);

		// Set blendSpec
		void ResetBlendSpecification(BlendSpecification& bs, const f32 timeDelta, const f32 currentValue, const f32 targetValue = 1.f);

		// Transition out old target set
		void TransitionOutClips(ClipSet& set, const f32 transitionStart, const f32 transitionLen);

		// normalize weight and retun clipCount in setter
		u32 PreProcessSetter(Setter& setter);

		// Reset a setter
		void ResetSetter(Setter& setter);
	};
}