#pragma once

namespace DOG
{
	struct RigSpecifics
	{
		u8 nJoints;
		u8 rootJoint;
		std::pair<u8, u8> groupMasks[3];
	};

	static constexpr u8 fullBodyGroup = 0;
	static constexpr u8 groupA = 1;
	static constexpr u8 groupB = 2;

	static constexpr u8 N_RIGS = 1;
	static constexpr u8 MIXAMO_RIG_ID = 0;
	static constexpr u8 SCRPIO_RIG_ID = 1;

	static constexpr RigSpecifics RIG_SPECIFICS[N_RIGS]{ { 65, 4, {std::make_pair<u8, u8>(0, 67), std::make_pair<u8, u8>(57, 10), std::make_pair<u8, u8>(5, 52) }} };
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
	//static constexpr RigSpecifics SCRPIO_RIG = RIG_SPECIFICS[SCRPIO_RIG_ID];

	struct Animator
	{
		static constexpr u8 maxClips = 10;
		static constexpr u8 groupA = 0; // Lower body 
		static constexpr u8 groupB = 1; // Upper body
		static constexpr u8 groupC = 2; // Full body
		static constexpr u8 noGroup = 3;
		static constexpr u8 nGroups = 3;
		static constexpr i8 NO_ANIMATION = -1;
		i32 offset = 0;
		f32 globalTime = 0.0f;
	
		struct ClipRigData
		{
			u32 aID;
			f32 weight;
			f32 tick;
		};
		struct GroupBlendSpec
		{
			f32 duration = 0.f;
			f32 startWeight = 0.f;
			f32 targetWeight = 0.f;
			f32 transitionStart = 0.f;
			f32 transitionLength = 0.f;
			bool Activated(const f32 gt, const f32 dt) const;
		};
		struct AnimationClip
		{
			static constexpr i32 noAnimation = -1;
			u8 group = 3;
			// Animation Specifics
			bool activeAnimation = false;
			i8 animationID = noAnimation;
			f32 transitionStart = 0.0f;
			f32 normalizedTime = 0.f;	// Tick
			f32 timeScale = 1.0f;		// Tick
			f32 duration = 1.0f;		// Tick
			f32 totalTicks = 1.0f;		// Tick
			bool loop = false;			// Tick
	
			f32 currentTick = 0.0f;
	
			f32 startWeight = 0.0f;		// Weight
			f32 targetWeight = 1.0f;	// Weight
			f32 transitionLength = 0.0f;// Weight
			f32 currentWeight = 0.0f;	// Weight
	
			// Update animation tick of clip
			f32 UpdateClipTick(const f32 transitionTime);
			bool HasActiveAnimation() const { return animationID != noAnimation; };
			// Set animation specifics fetched from rig
			void SetAnimation(const f32 animationDuration, const f32 nTicks);
			// True if clip activated this frame
			bool Activated(const f32 gt, const f32 dt) const;
			// True if clip deactivated this frame
			bool Deactivated() const;
			// Update state of clip
			void UpdateState(const f32 gt);
			bool operator <(const AnimationClip& o) const {
				return activeAnimation && !o.activeAnimation ||
					activeAnimation == o.activeAnimation && group < o.group ||
					activeAnimation == o.activeAnimation && group == o.group && currentWeight > o.currentWeight ||
					activeAnimation == o.activeAnimation && group == o.group && currentWeight == o.currentWeight && targetWeight > o.targetWeight;
			}
			bool operator ==(const AnimationClip& o) const {
				return animationID == o.animationID && group == o.group;
			}
		};
		std::array<AnimationClip, maxClips> clips;
		// Number of new clips added to component this frame
		u32 nAddedClips = 0;
		// Active clips per group
		u8 clipsInGroup[nGroups] = { 0 };
	
		ClipRigData clipData[maxClips];
		// weight between 
		std::array<GroupBlendSpec, 4> groups;
		f32 groupWeights[nGroups - 1] = { .0f };
		// Update clip weights and ticks
		void Update(const f32 dt);
		// Group Blend Specifics
		f32 LinearBlend(const f32 currentTime, const f32 stopTime, const f32 startValue, const f32 targetValue, f32 currentValue = 0.0f) const;
		f32 BezierBlend(const f32 currentTime, const f32 stopTime, const f32 startValue, const f32 targetValue, f32 currentValue = 0.0f) const;
		// Overwrites an active clip with values from newly activated clip if applicable
		bool ReplacedClip(AnimationClip& clip, u32 idx);
		// Add a new animation clip to components timeline
		void AddAnimationClip(i8 id, f32 duration, f32 ticks, u8 group, f32 transitionLength, f32 startWeight, f32 targetWeight, bool loop = false, f32 timeScale = 1.f, f32 startDelay = 0.f);
		void AddBlendSpecification(f32 startDelay, f32 transitionLength, u32 group, f32 targetWeight, f32 duration = -1.f);
		// Returns number of currently active clips contributing to current pose
		i32 ActiveClipCount() const;
		// Returns index of first clip in group
		u8 GetGroupIndex(const u8 group) const;
		// Return number of clips on the timeline
		i32 ClipCount() const;

		void ResetClip(AnimationClip& clip);
		bool OverwriteClip(AnimationClip& c);
		u32 ClipIdx(const u8 group, const u8 animationID);
	};
}

