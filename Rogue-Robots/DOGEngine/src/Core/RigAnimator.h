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
		std::pair<u8, u8> groupMasks[3];
		u8 groupParent[3] = { 0 };
	};

	static constexpr u8 fullBodyGroup = 0;
	static constexpr u8 groupA = 1;
	static constexpr u8 groupB = 2;

	static constexpr u8 N_RIGS = 1;
	static constexpr u8 MIXAMO_RIG_ID = 0;
	//static constexpr u8 SCRPIO_RIG_ID = 1;

	static constexpr RigSpecifics RIG_SPECIFICS[N_RIGS]{ { 66, 4, {std::make_pair<u8, u8>(0, 67), std::make_pair<u8, u8>(57, 11), std::make_pair<u8, u8>(5, 53) }, { 0, 0, 0 }} };
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
	struct testAnimator
	{
		ImportedRig* rigData = {};

		f32 globalTime = 0.f;
		using Setter = AnimationComponent::Setter;
		struct Bs
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
			Bs blend = {};
			std::array<Clip, 6> clips;
		};
		struct Group
		{
			u32 priority = BASE_PRIORITY;
			i32 parent = -1;
			f32 weight = 0.f;
			Bs blend = {};
			ClipSet looping;
			ClipSet action;
		};
		std::array<Group, N_GROUPS> groups = {};
		std::array<Bs, N_GROUPS> groupBlends = {};
		std::array<u32, N_GROUPS> groupClipCount = { 0 };
		std::array<ClipData, N_GROUPS * 12> clipData = {};

		Bs testBs = {};
		testAnimator()
		{
			testBs.durationLeft = 0.f;
			testBs.playbackRate = 1.f;
			testBs.transitionStart = 0.f;
			testBs.transitionLength = 0.f;
			testBs.startWeight = 1.f;
			testBs.currentWeight = 1.f;
			testBs.targetWeight = 1.f;
		}
		bool GroupHasLooping(const Group& g)
		{
			return g.looping.nTotalClips;
		}
		bool TransitionIsFaster(const f32 start, const f32 tl, const f32 transitionStart, const f32 transitionLen)
		{
			return start + tl < transitionStart + transitionLen;
		}
		bool HasLooping(AnimationFlag flags)
		{
			return static_cast<bool>(flags & AnimationFlag::Looping);
		}
		bool HasPersist(AnimationFlag flags)
		{
			return static_cast<bool>(flags & AnimationFlag::Persist);
		}
		bool HasResetPrio(AnimationFlag flags)
		{
			return static_cast<bool>(flags & AnimationFlag::ResetPrio);
		}
		void AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime = 0.f)
		{
			// Add clip to back
			const auto clipIdx = std::clamp(set.nTotalClips, 0u, MAX_CLIPS - 1);
			auto& clip = set.clips[clipIdx];
			clip.id = setter.animationIDs[setIdx]; // this is poop
			clip.duration = rigData->animations[clip.id].duration;
			clip.totalTicks = rigData->animations[clip.id].ticks;
			clip.currentWeight = 0.f;
			clip.normalizedTime = startTime;
			clip.targetWeight = setter.targetWeights[setIdx];
			clip.timeScale = setter.playbackRate;
			// Swap clip to desired position
			std::swap(set.clips[setIdx], set.clips[clipIdx]);
			set.nTotalClips = std::clamp(++set.nTotalClips, 1u, MAX_CLIPS);

			if (set.nTotalClips == MAX_CLIPS)
			{// Sort such that least contributing clip would be removed should another clip get added
				std::sort(set.clips.begin() + MAX_TARGETS, set.clips.begin() + MAX_CLIPS,
					[](const Clip& a, const Clip& b) -> bool
					{
						return a.targetWeight < b.targetWeight;
					});
			}
		}
		void ModifyClip(Clip& clip, Setter& setter, u32 setIdx)
		{
			clip.targetWeight = setter.targetWeights[setIdx];
			clip.timeScale = setter.playbackRate;
			clip.normalizedTime = clip.normalizedTime; // if (force_restart) nt = 0.f
		}
		void UpdateClipCW(ClipSet& set, u32 idx)
		{
			const bool clipIsTarget = idx < set.nTargets;
			const auto weight = clipIsTarget ? set.blend.currentWeight : 1.f - set.blend.currentWeight;

			auto& clip = set.clips[idx];
			clip.currentWeight = weight * (clip.targetWeight - clip.currentWeight) + clip.currentWeight;
		}
		void PostAddFixUp(ClipSet& set, u32 idx)
		{
			const bool targetClipSwapped = idx < set.nTargets;
			if (targetClipSwapped)
			{
				auto& lastClip = set.clips[set.nTotalClips - 1];
				const auto weight = set.blend.currentWeight;
				lastClip.targetWeight = weight * (lastClip.targetWeight - lastClip.currentWeight) + lastClip.currentWeight;
				lastClip.currentWeight = 0.f;
			}
			auto& lastClip = set.clips[set.nTotalClips - 1];
			for (u32 i = set.nTargets; i < set.nTotalClips - 1; ++i)
			{
				auto& c = set.clips[i];
				if (c.id == lastClip.id)
				{
					c.targetWeight += lastClip.targetWeight;
					set.nTotalClips--;
				}
			}
		}
		void AddTargetSet(Setter& setter, ClipSet& set, u32 clipCount, bool looping)
		{
			static constexpr i32 NOT_FOUND = -1;
			f32 startingTime = set.nTotalClips && looping ? set.clips[0].normalizedTime : 0.f;

			for (u32 i = 0; i < clipCount; ++i)
			{
				const auto clipIdx = GetClipIndex(set, setter.animationIDs[i]);

				if (NOT_FOUND == clipIdx || clipIdx < static_cast<i32>(set.nTargets))
				{ // Clip not part of set, add it
					AddClip(set, setter, i, startingTime);
					PostAddFixUp(set, i);
				}
				else
				{ // Clip exists, modify it
					UpdateClipCW(set, clipIdx);
					ModifyClip(set.clips[clipIdx], setter, i);
					std::swap(set.clips[i], set.clips[clipIdx]);
				}
			}
			set.nTargets = clipCount;
		}

		std::array<f32, 6> GetWeights(ClipSet& set)
		{
			f32 bw = set.nTargets == set.nTotalClips ? 1.0f : set.blend.currentWeight;

			std::array<f32, 6> tmp = { -1.f };
			for (u32 i = 0; i < set.nTotalClips; i++)
			{
				auto& clip = set.clips[i];
				if (i < set.nTargets)
				{
					tmp[i] = bw * (clip.targetWeight - clip.currentWeight) + clip.currentWeight;
				}
				else
				{
					tmp[i] = clip.currentWeight = (1.f - bw) * clip.targetWeight;
				}
			}
			for (size_t i = 0; i < set.nTotalClips; i++)
				assert(tmp[i] >= 0.f);

			return tmp;
		}

		i32 GetClipIndex(const ClipSet& set, const i32 animationID)
		{	// Returns clip index if animation is present in the set
			static constexpr i32 NOT_FOUND = -1;
			for (u32 i = 0; i < set.nTotalClips; ++i)
				if (set.clips[i].id == animationID)
					return i;
			return NOT_FOUND;
		}
		f32 LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue)
		{
			if (currentTime >= transitionLength) // Transition is done
				currentValue = targetValue;
			else if (currentTime > 0.0f) // Linear
				currentValue = startValue + currentTime * (targetValue - startValue) / transitionLength;
			return currentValue;
		}
		f32 BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue)
		{
			if (currentTime >= transitionLength) // Transition is done
				currentValue = targetValue;
			else if (currentTime > 0.0f) // bezier curve
			{
				const f32 u = currentTime / transitionLength;
				const f32 v = 1.0f - u;
				currentValue = startValue * (powf(v, 3) + 3 * powf(v, 2) * u) +
					targetValue * (3 * v * powf(u, 2) + powf(u, 3));
			}
			return currentValue;
		}

		void SetReturnTransition(Bs& bs, f32 delta)
		{
			bs.durationLeft = 0.f;
			bs.transitionStart = globalTime + delta;
			bs.startWeight = bs.currentWeight;
			bs.targetWeight = 0.f;
		}

		void SetBS(Bs& bs, f32 transitionLen, f32 targetWeight, f32 startWeight = 0.f, f32 duration = 0.f)
		{
			bs.durationLeft = duration;
			bs.transitionStart = globalTime;
			bs.transitionLength = transitionLen;
			bs.startWeight = startWeight;
			bs.targetWeight = targetWeight;
		}

		void UpdateBsWeight(Bs& bs, f32 dt)
		{
			if (dt < 0.f)
				return;
			const bool targetReached = bs.currentWeight == bs.targetWeight;
			if (!targetReached)
			{
				auto currentTime = globalTime - bs.transitionStart;

				const bool transitionStarted = currentTime > 0.f;
				if (transitionStarted)
					bs.currentWeight = BezierWeight(currentTime, bs.transitionLength, bs.startWeight, bs.targetWeight, bs.currentWeight); // flag transtitonTyhpe : bezier()
			}
		}

		void UpdateBsDuration(Bs& bs, f32 dt)
		{
			const bool durationLeft = bs.durationLeft > 0.f;
			if (durationLeft)
			{
				bs.durationLeft -= dt;

				const bool setupReturnTranstition = bs.durationLeft < bs.transitionLength;
				if (setupReturnTranstition)
				{
					SetReturnTransition(bs, bs.durationLeft - bs.transitionLength);
				}
			}
		}

		void UpdateBS(Bs& bs, f32 dt)
		{
			UpdateBsDuration(bs, dt);
			UpdateBsWeight(bs, dt);
		}

		void UpdateSet(ClipSet& set, u32 clipIdx, f32 blendWeight, f32 dt, bool looping = false)
		{
			//UpdateBS(set.blend, dt);
			const auto weights = GetWeights(set);

			ClipNormalizedTime(set.clips[0], dt, looping);
			const auto targetNt = set.clips[0].normalizedTime;
			for (u32 i = 0; i < set.nTotalClips; i++)
			{
				auto& c = set.clips[i];
				const bool clipIsTarget = i < set.nTargets;
				c.normalizedTime = clipIsTarget ? targetNt : ClipNormalizedTime(c, dt, looping);

				auto& cd = clipData[clipIdx++];
				cd.aID = c.id;
				cd.tick = ClipTick(c);
				cd.weight = blendWeight * weights[i];
			}
		}

		u32 UpdateGroup(u32 groupIdx, u32 startClipIdx, f32 dt)
		{
			auto& group = groups[groupIdx];
			auto& blend = group.blend;
			//UpdateBS(blend, dt);

			auto clipCount = 0u;
			const f32 actionSetWeight = (group.looping.nTotalClips > 0) ? blend.currentWeight : 1.f;
			const f32 looperSetWeight = 1.f - actionSetWeight;

			const bool actionSetHasInfluence = actionSetWeight > 0.f;
			if (actionSetHasInfluence)
			{
				UpdateSet(group.action, startClipIdx, actionSetWeight, dt);
				clipCount += group.action.nTotalClips;
			}

			UpdateSet(group.looping, startClipIdx + clipCount, looperSetWeight, dt, true);
			clipCount += group.looping.nTotalClips;

			// Debug
			f32 sum = 0.f;
			for (u32 i = startClipIdx; i < clipCount; i++)
			{
				sum += clipData[i].weight;
			}

			groupClipCount[groupIdx] = clipCount;
			return startClipIdx + clipCount;
		}

		void ResetGroup(Group& group)
		{
			group.blend.durationLeft = 0.f;
			group.blend.targetWeight = group.blend.currentWeight = 0;
			group.action.nTargets = group.action.nTotalClips = 0;
			group.looping.nTargets = group.looping.nTotalClips = 0;
		}

		f32 ClipNormalizedTime(Clip& c, const f32 dt, bool loop)
		{
			c.normalizedTime += dt * c.timeScale / c.duration;

			c.normalizedTime = loop ?
				fmodf(c.normalizedTime, 1.0f) :
				std::clamp(c.normalizedTime, 0.0f, 1.0f);

			return c.normalizedTime;
		};

		f32 ClipTick(Clip& c)
		{
			return c.normalizedTime * c.totalTicks;
		};
		f32 ClipTick(Clip& c, const f32 dt, bool loop)
		{
			c.normalizedTime += dt * c.timeScale / c.duration;

			c.normalizedTime = loop ?
				fmodf(c.normalizedTime, 1.0f) :
				std::clamp(c.normalizedTime, 0.0f, 1.0f);
			return c.normalizedTime * c.totalTicks;
		};

		void ProcessAnimationComponent(AnimationComponent& ac)
		{
			// Sort added setters by group and loop status
			std::sort(ac.animSetters.begin(), ac.animSetters.begin() + ac.addedSetters,
				[](const Setter& a, const Setter& b) -> bool
				{
					return a.group < b.group || (a.group == b.group &&
						static_cast<bool>(a.flag & AnimationFlag::Looping) && !static_cast<bool>(b.flag & AnimationFlag::Looping));
				});

			for (i32 i = 0; i < ac.addedSetters; ++i)
			{
				auto& setter = ac.animSetters[i];

				ProcessSetter(setter);
				ResetSetter(setter);
			}
			ac.addedSetters = 0;
		}

		u32 PreProcessSetter(Setter& setter)
		{
			static constexpr i32 INVALID = -1;
			auto& set = HasLooping(setter.flag) ? groups[setter.group].looping : groups[setter.group].action;
			u32 clipCount = 0;
			f32 wSum = 0.f;
			
			// Get clip count and target weight sum
			for (u32 i = 0; i < MAX_TARGETS; ++i)
			{
				if (setter.animationIDs[i] == INVALID)
					break;

				++clipCount;
				// targetweights must be larger than 0
				setter.targetWeights[i] = setter.targetWeights[i] <= 0.f ? 0.1f : setter.targetWeights[i];
				wSum += setter.targetWeights[i];
			}
			// Normalize target weights
			for (u32 i = 0; i < clipCount; i++)
				setter.targetWeights[i] /= wSum;

			// Dont set if same as current target set
			bool newTargetSet = set.nTargets == 0;
			for (u32 i = 0; i < MAX_TARGETS; i++)
			{
				if (setter.animationIDs[i] == INVALID || newTargetSet)
				{
					if (i != set.nTargets)
						newTargetSet = true;
					break;
				}
				newTargetSet = (set.clips[i].targetWeight != setter.targetWeights[i]) || (set.clips[i].id != setter.animationIDs[i]);
			}
			// same target set already set
			if (!newTargetSet)
				return 0;

			return clipCount;
		}

		void ProcessSetter(Setter& setter)
		{
			const auto groupIdx = setter.group;
			auto& group = groups[groupIdx];
			// Normalize weights and get number of clips in setter
			u32 clipCount = PreProcessSetter(setter);

			const bool lowerPrio = !HasResetPrio(setter.flag) && setter.priority < group.priority;
			const bool invalidSetter = (!clipCount || lowerPrio);
			if (invalidSetter)
				return;

			// Update priority
			group.priority = static_cast<u32>(setter.priority);

			// Get corresponding set
			auto& set = HasLooping(setter.flag) ?
				group.looping : group.action;

			// Add/modify the target
			AddTargetSet(setter, set, clipCount, HasLooping(setter.flag));

			SetBlendSpecifications(setter);
		}
		void SetBlendSpecifications(Setter& setter)
		{
			SetClipsBS(setter);
			SetSetBS(setter);
			SetGroupBS(setter);
		}

		void SetClipsBS(Setter& setter)
		{
			auto& group = groups[setter.group];
			auto& bs = HasLooping(setter.flag) ? group.looping.blend : group.action.blend;
			bs.currentWeight = 0.f;
			bs.targetWeight = 1.f;
			bs.transitionStart = globalTime;
			bs.transitionLength = setter.transitionLength;
		}
		void SetSetBS(Setter& setter)
		{
			auto& bs = groups[setter.group].blend;
			if (!HasLooping(setter.flag)) // more logic needed here
			{
				if (!HasPersist(setter.flag))
				{
					bs.durationLeft = groups[setter.group].action.clips[0].duration / setter.playbackRate;
				}
				bs.startWeight = bs.currentWeight;
				bs.targetWeight = 1.f;
				bs.transitionStart = globalTime;
				bs.transitionLength = setter.transitionLength;
			}
			else if (bs.currentWeight == bs.targetWeight == 1.f)
			{
				bs.startWeight = bs.currentWeight;
				bs.targetWeight = 0.f;
				bs.transitionStart = globalTime;
				bs.transitionLength = setter.transitionLength;
			}
		}
		void SetGroupBS(Setter& setter)
		{
			auto& bs = groupBlends[setter.group];
			bool php = !ParentHigherPrio(setter.group);
			const bool updateBS = php && (bs.targetWeight == 0.f || TransitionIsFaster(globalTime, setter.transitionLength, bs.transitionStart, bs.transitionLength));
			if (updateBS)
			{
				if (!HasLooping(setter.flag) && !HasPersist(setter.flag))
				{
					bs.durationLeft = groups[setter.group].action.clips[0].duration / setter.playbackRate;
				}
				bs.startWeight = bs.currentWeight;
				bs.targetWeight = 1.f;
				bs.transitionStart = globalTime;
				bs.transitionLength = setter.transitionLength;
			}
		}

		void SetGroupBS(Setter& setter, f32 duration)
		{
			if (!groupClipCount[setter.group])
			{
				auto& bs = groupBlends[setter.group];
				bs.durationLeft = duration;
				bs.startWeight = bs.currentWeight;
				bs.targetWeight = 1.f;
				bs.transitionStart = globalTime;
				bs.transitionLength = setter.transitionLength;
			}
		}

		void ResetSetter(Setter& setter)
		{
			for (u32 i = 0; i < MAX_TARGETS; i++)
				setter.animationIDs[i] = -1;
			setter.flag = AnimationFlag::None;
		}

		f32 GetGroupWeight(u32 group)
		{
			auto parent = groups[group].parent;
			const auto prio = groups[group].priority;
			const auto weight = groups[group].weight;

			static constexpr i16 origin = -1;
			// Travel group tree, if a parent group has higher prio group has no influence
			while (parent != origin)
				if (prio < groups[parent].priority)
					return 0.f;
				else
					parent = groups[parent].parent;

			return weight;
		}
		bool ParentHigherPrio(u32 group)
		{
			auto parent = groups[group].parent;
			const auto prio = groups[group].priority;

			bool parentHigherPrio = false;
			static constexpr i16 origin = -1;
			while (parent != origin && !parentHigherPrio)
			{
				parentHigherPrio = groups[parent].priority > prio;
				parent = groups[parent].parent;
			}
			return parentHigherPrio;
		}

		u32 GetGroupStartIdx(u32 group)
		{
			u32 idx = 0;
			for (u32 i = 0; i < group; i++)
				idx += groupClipCount[i];
			return idx;
		}

		void Update(f32 dt)
		{
			globalTime += dt; // * globalPlaybackRate ?
			for (u32 i = 1; i < groups.size(); ++i)
			{
				UpdateBS(groupBlends[i], dt);
				groups[i].weight = groupBlends[i].currentWeight;

				if (groups[i].weight <= 0.f)
					ResetGroup(groups[i]);
			}
			u32 clipIdx = 0;
			for (u32 i = 0; i < groups.size(); ++i)
			{
				auto& group = groups[i];
				UpdateBS(group.blend, dt);
				if (group.blend.currentWeight == 0.f)
					group.action.nTargets = group.action.nTotalClips = 0;

				UpdateBS(group.action.blend, dt);
				if (group.action.blend.currentWeight == 1.f)
					group.action.nTotalClips = group.action.nTargets;

				UpdateBS(group.looping.blend, dt);
				if (group.looping.blend.currentWeight == 1.f)
					group.looping.nTotalClips = group.looping.nTargets;


				clipIdx = UpdateGroup(i, clipIdx, dt);
			}
		}
	};
}