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
		f32 scale = 0.f;
		f32 duration = 0.f;
		f32 transitionStart = 0.f;
		f32 transitionLength = 0.f;
		f32 startWeight = 1.f;
	};
	// Set of active clips,
	struct ClipSet
	{
		f32 weight = 0.f;
		f32 playbackRate = 0.f;
		u32 startClip = 0;
		u32 nTargetClips = 0;
		u32 nTotalClips = 0;
	};
	// Group of clips influencing rig, contains a 'looping' clipset and a 'action' clipset, and a blendspecification between them
	struct AnimationGroup
	{
		u32 primary = 0;
		f32 weight = {};
		BlendSpecification blend;
		std::array<ClipSet, 2> sets;
	};
	// Animator of a rigged entity, handles adding clips via RigAnimationComponent and updating/removing active clips
	struct RigAnimator
	{
		using Setter = AnimationComponent::Setter2;
		
		f32 globalTime = 0.0f;
		ImportedRig* rigData = {};

		std::array<Clip, 50> clips = {};
		// Weights per group, fullbody 
		f32 gWeights[N_GROUPS - 1] = { 0.5f, 0.f };
		// Number of Clips influencing group
		std::array<u32, N_GROUPS> groupClipCount = { 0 };
		// Data needed to update rig
		std::array<ClipData, 50> clipData = {};
		// Animation groups
		std::array<AnimationGroup, N_GROUPS> groups = {};

		RigAnimator();

		// Update active clips
		void Update(const f32 dt);

		// Get index of clip
		i32 GetClipIndex(const ClipSet& set, const i32 animationID);

		// Add a new clip
		void AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime);

		// Modifies an active clip
		void ModifyClip(Clip& clip, Setter& setter, u32 setIdx);

		// Set new target animation set
		void AddTargetSet(ClipSet& set, Setter& setter, u32 clipCount);

		// Get index of first clip from group
		u32 GetGroupStartIdx(u32 group);

		// Updates the blend spec and clips in group
		u32 UpdateGroup(AnimationGroup& group, const u32 clipIdx, const f32 dt);

		// Updates clips in set
		u32 UpdateClipSet(ClipSet& set, const u32 clipIdx, const f32 dt, bool looping = false);

		// Update and return normalized time of a clip
		f32 ClipNormalizedTime(Clip& c, const f32 delta, bool loop);

		// Get current tick of a clip
		f32 ClipTick(const Clip& c, const f32 nt);

		// Blend functions
		f32 BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue);
		f32 LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue);

		// Process the AnimationComponent and the Setters within, adding/modifying active clips that influences the entity
		void HandleAnimationComponent(AnimationComponent& ac);
		void HandleSetter(Setter& setter, AnimationGroup& group);

		// Set clip data based on setter
		void SetClip(Setter& setter, u32 setIdx, Clip& clip);

		// Set blendSpec based on setter
		void SetBlendSpec(AnimationGroup& group, const Setter& setter);

		// Update group blendSpec and setting corresponding weights
		void UpdateBlendSpec(AnimationGroup& group, const f32 dt);

		// Transition out old target set
		void TransitionOutClips(ClipSet& set, const f32 transitionStart, const f32 transitionLen);

		// Assert that setter is valid input
		u32 ValidateSetter(Setter& setter);

		// Reset a setter
		void ResetSetter(Setter& setter);
	};
}