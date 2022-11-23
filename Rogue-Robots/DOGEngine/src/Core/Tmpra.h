#pragma once
#include "../ECS/Component.h"
#include "Types/AssetTypes.h"

namespace DOG
{
	// Animator of a rigged entity, handles adding clips via RigAnimationComponent and updating/removing active clips
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
		struct ClipAnimationData
		{
			i32 id = -1;
			f32 weight = 0.f;
			f32 tick = 0.f;
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
		std::array<ClipData, N_GROUPS*12> clipData = {};

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
		bool TransitionIsFaster(const f32 start, const f32 tl, const Bs& bs)
		{
			return start + tl < bs.transitionStart + bs.transitionLength;
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
		}
		void AddTargetSet(Setter& setter, ClipSet& set, u32 clipCount)
		{
			static constexpr i32 NOT_FOUND = -1;
			for (u32 i = 0; i < clipCount; ++i)
			{
				const auto clipIdx = GetClipIndex(set, setter.animationIDs[i]);

				if (NOT_FOUND == clipIdx)
				{ // Clip not part of set, add it
					AddClip(set, setter, i);
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
					bs.currentWeight = LinearWeight(currentTime, bs.transitionLength, bs.startWeight, bs.targetWeight, bs.currentWeight); // flag transtitonTyhpe : bezier()
			}
		}

		void UpdateBsDuration(Bs& bs, f32 dt)
		{
			if (bs.durationLeft > 0.f)
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
			if (clipCount)
			{
				if (dt < 0.f)
				{
					return 0;
				}
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
			// sort added setters by group and loop status
			std::sort(ac.animSetters.begin(), ac.animSetters.begin() + ac.addedSetters,
				[](const Setter& a, const Setter& b) -> bool
				{
					return a.group < b.group || (a.group == b.group && a.loop && !b.loop);
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
			auto& set = setter.loop ? groups[setter.group].looping : groups[setter.group].action;
			u32 clipCount = 0;
			f32 wSum = 0.f;
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

			return clipCount;
		}

		void ProcessSetter(Setter& setter)
		{
			const auto groupIdx = setter.group;
			auto& group = groups[groupIdx];
			// Normalize weights and get number of clips in setter
			u32 clipCount = PreProcessSetter(setter);

			const bool invalidSetter = (!clipCount || setter.priority < group.priority);
			if (invalidSetter)
				return;

			// Update priority
			group.priority = static_cast<u32>(setter.priority);

			// Get corresponding set
			auto& set = setter.loop ?
				group.looping : group.action;

			// Add/modify the target
			AddTargetSet(setter, set, clipCount);

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
			auto& bs = setter.loop ? group.looping.blend : group.action.blend;
			bs.currentWeight = 0.f;
			bs.targetWeight = 1.f;
			bs.transitionStart = globalTime;
			bs.transitionLength = setter.transitionLength;
		}
		void SetSetBS(Setter& setter)
		{
			if (!setter.loop) // more logic needed here
			{
				auto& bs = groups[setter.group].blend;
				if (!(bool)(setter.flag & AnimationFlag::Persist))
				{
					bs.durationLeft = groups[setter.group].action.clips[0].duration / setter.playbackRate;
				}
				bs.currentWeight = bs.startWeight = 0.f;
				bs.targetWeight = 1.f;
				bs.transitionStart = globalTime;
				bs.transitionLength = setter.transitionLength;
			}
		}
		void SetGroupBS(Setter& setter)
		{
			auto& bs = groupBlends[setter.group];
			bool php = !ParentHigherPrio(setter.group);
			const bool updateBS = php && (bs.targetWeight == 0.f || TransitionIsFaster(globalTime, setter.transitionLength, bs));
			if (updateBS)
			{
				if (!setter.loop && !(bool)(setter.flag & AnimationFlag::Persist))
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