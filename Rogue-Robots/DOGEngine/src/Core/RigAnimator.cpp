#include "RigAnimator.h"

namespace DOG
{
	RigAnimator::RigAnimator()
	{}

	bool GroupHasLooping(const RigAnimator::Group& g)
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
	bool HasInterrupt(AnimationFlag flags)
	{
		return static_cast<bool>(flags & AnimationFlag::Interrupt);
	}
	bool HasForceRestart(AnimationFlag flags)
	{
		return static_cast<bool>(flags & AnimationFlag::ForceRestart);
	}
	f32 ClipTick(RigAnimator::Clip& c)
	{
		return c.normalizedTime * c.totalTicks;
	};

	void RigAnimator::AddTargetSet(Setter& setter, ClipSet& set, u32 clipCount, bool looping)
	{
		static constexpr i32 NOT_FOUND = -1;
		f32 startingTime = set.nTotalClips && looping ? set.clips[0].normalizedTime : 0.f;

		for (u32 i = 0; i < clipCount; ++i)
		{
			const auto clipIdx = GetClipIndex(set, setter.animationIDs[i]);

			if (NOT_FOUND == clipIdx)
			{ // Clip not part of set, add it
				AddClip(set, setter, i, startingTime);
				PostAddFixUp(set, i);
			}
			else
			{ // Clip exists, modify it
				UpdateClipCW(set, clipIdx, clipCount);
				ModifyClip(set.clips[clipIdx], setter, i);
				std::swap(set.clips[i], set.clips[clipIdx]);
			}
		}
		set.nTargets = clipCount;
	}
	void RigAnimator::AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime)
	{
		// Add clip to back
		const auto clipIdx = std::clamp(set.nTotalClips, 0u, MAX_CLIPS - 1);
		auto& clip = set.clips[clipIdx];
		clip.id = setter.animationIDs[setIdx];
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
	void RigAnimator::PostAddFixUp(ClipSet& set, u32 idx)
	{
		const bool targetClipSwapped = idx < set.nTargets;
		if (targetClipSwapped)
		{
			auto lastClipIdx = set.nTotalClips - 1;
			auto& lastClip = set.clips[lastClipIdx];
			const auto weight = set.blend.currentWeight;
			lastClip.targetWeight = weight * (lastClip.targetWeight - lastClip.currentWeight) + lastClip.currentWeight;
			lastClip.currentWeight = 0.f;
		}
	}
	void RigAnimator::ModifyClip(Clip& clip, Setter& setter, u32 setIdx)
	{
		clip.targetWeight = setter.targetWeights[setIdx];
		clip.timeScale = setter.playbackRate;
		clip.normalizedTime = clip.normalizedTime;
		if (HasForceRestart(setter.flag))
		{
			clip.normalizedTime = 0.f;
		}
	}
	void RigAnimator::UpdateClipCW(ClipSet& set, u32 idx, u32 newNrOfTargets)
	{
		const bool clipIsTarget = idx < newNrOfTargets;
		const auto weight = clipIsTarget ? set.blend.currentWeight : 1.f - set.blend.currentWeight;

		auto& clip = set.clips[idx];
		clip.currentWeight = weight * (clip.targetWeight - clip.currentWeight) + clip.currentWeight;
	}

	std::array<f32, 6> RigAnimator::GetWeights(ClipSet& set)
	{
		const auto bw = set.blend.currentWeight;

		auto weightSum = 0.f;
		std::array<f32, 6> weights = { -1.f };
		for (u32 i = 0; i < set.nTotalClips; i++)
		{
			auto& clip = set.clips[i];
			if (i < set.nTargets)
			{
				weights[i] = bw * (clip.targetWeight - clip.currentWeight) + clip.currentWeight;
			}
			else
			{
				weights[i] = clip.currentWeight = (1.f - bw) * clip.targetWeight;
			}
			weightSum += weights[i];
		}

		if (weightSum != 1.f)
			for (u32 i = 0; i < set.nTotalClips; i++)
				weights[i] /= weightSum;

		return weights;
	}

	i32 RigAnimator::GetClipIndex(const ClipSet& set, const i32 animationID)
	{	// Returns clip index if animation is present in the set
		static constexpr i32 NOT_FOUND = -1;
		for (u32 i = 0; i < set.nTotalClips; ++i)
			if (set.clips[i].id == animationID)
				return i;
		return NOT_FOUND;
	}
	f32 RigAnimator::LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue)
	{
		if (currentTime >= transitionLength) // Transition is done
			currentValue = targetValue;
		else if (currentTime > 0.0f) // Linear
			currentValue = startValue + currentTime * (targetValue - startValue) / transitionLength;
		return currentValue;
	}
	f32 RigAnimator::BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue)
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

	void RigAnimator::SetReturnTransition(BlendSpecification& bs, f32 delta)
	{
		bs.durationLeft = 0.f;
		bs.transitionStart = globalTime + delta;
		bs.startWeight = bs.currentWeight;
		bs.targetWeight = 0.f;
	}

	void RigAnimator::UpdateBsWeight(BlendSpecification& bs)
	{
		const bool targetReached = bs.currentWeight == bs.targetWeight;
		if (!targetReached)
		{
			auto currentTime = globalTime - bs.transitionStart;

			const bool transitionStarted = currentTime > 0.f;
			if (transitionStarted)
				bs.currentWeight = BezierWeight(currentTime, bs.transitionLength, bs.startWeight, bs.targetWeight, bs.currentWeight); // flag transtitonTyhpe : bezier()
		}
	}

	void RigAnimator::UpdateBsDuration(BlendSpecification& bs, f32 dt)
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

	void RigAnimator::UpdateBS(BlendSpecification& bs, f32 dt)
	{
		UpdateBsDuration(bs, dt);
		UpdateBsWeight(bs);
	}

	void RigAnimator::UpdateSet(ClipSet& set, u32 clipIdx, f32 blendWeight, f32 dt, bool looping)
	{
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

	u32 RigAnimator::UpdateGroup(u32 groupIdx, u32 startClipIdx, f32 dt)
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

		groupClipCount[groupIdx] = clipCount;
		return startClipIdx + clipCount;
	}

	void RigAnimator::ResetGroup(Group& group)
	{
		group.blend.durationLeft = 0.f;
		group.blend.targetWeight = group.blend.currentWeight = 0;
		group.action.nTargets = group.action.nTotalClips = 0;
		group.looping.nTargets = group.looping.nTotalClips = 0;
	}

	f32 RigAnimator::ClipNormalizedTime(Clip& c, const f32 dt, bool loop)
	{
		c.normalizedTime += dt * c.timeScale / c.duration;

		c.normalizedTime = loop ?
			fmodf(c.normalizedTime, 1.0f) :
			std::clamp(c.normalizedTime, 0.0f, 1.0f);

		return c.normalizedTime;
	};

	void RigAnimator::ProcessAnimationComponent(AnimationComponent& ac)
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

	u32 RigAnimator::PreProcessSetter(Setter& setter)
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
		bool newTargets = false;
		bool newWeights = false;
		bool newTargetSet = set.nTargets == 0;
		for (u32 i = 0; i < MAX_TARGETS; i++)
		{
			if (setter.animationIDs[i] == INVALID || newTargetSet)
			{
				if (i != set.nTargets)
					newTargetSet = true;
				break;
			}
			if (!newWeights)
				newWeights = (set.clips[i].targetWeight != setter.targetWeights[i]);
			if (!newTargets)
				newTargets = (set.clips[i].id != setter.animationIDs[i]);

			newTargetSet = (newWeights) || (newTargets);
		}

		const bool newSetter = newTargetSet || (HasInterrupt(setter.flag) || HasForceRestart(setter.flag) || HasResetPrio(setter.flag));
		return newSetter * clipCount;
	}

	void RigAnimator::ProcessSetter(Setter& setter)
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
	void RigAnimator::SetBlendSpecifications(Setter& setter)
	{
		SetClipsBS(setter);
		SetSetBS(setter);
		SetGroupBS(setter);
		static constexpr i32 origin = -1;
		auto newPrio = setter.priority;
		for (u32 i = setter.group + 1; i < N_GROUPS; ++i)
		{
			auto parent = groups[i].parent;
			while (parent != origin)
			{
				if (parent == setter.group && groups[i].priority < newPrio)
					TransitionOutGroupBS(setter, groupBlends[i]);
				parent = groups[parent].parent;
			}
		}
	}
	void RigAnimator::TransitionOutGroupBS(Setter& setter, BlendSpecification& bs)
	{
		bs.startWeight = bs.currentWeight;
		bs.targetWeight = 0.f;
		bs.transitionStart = globalTime;
		bs.transitionLength = setter.transitionLength;
	}

	void RigAnimator::SetClipsBS(Setter& setter)
	{
		auto& group = groups[setter.group];
		auto& bs = HasLooping(setter.flag) ? group.looping.blend : group.action.blend;
		bs.currentWeight = HasInterrupt(setter.flag) ? 1.f : 0.f;
		bs.targetWeight = 1.f;
		bs.transitionStart = globalTime;
		bs.transitionLength = setter.transitionLength;
	}
	void RigAnimator::SetSetBS(Setter& setter)
	{
		auto& bs = groups[setter.group].blend;
		if (!HasLooping(setter.flag)) // more logic needed here
		{
			if (!HasPersist(setter.flag))
			{
				bs.durationLeft = groups[setter.group].action.clips[0].duration / setter.playbackRate;
			}
			bs.startWeight = bs.currentWeight;
			bs.targetWeight = TARGET_ACTION;
			bs.transitionStart = globalTime;
			bs.transitionLength = setter.transitionLength;
		}
		else if (HasInterrupt(setter.flag) || HasResetPrio(setter.flag))
		{
			bs.startWeight = HasInterrupt(setter.flag) ? TARGET_LOOPING : bs.currentWeight;
			bs.targetWeight = TARGET_LOOPING;
			bs.transitionStart = globalTime;
			bs.transitionLength = setter.transitionLength;
		}
	}
	void RigAnimator::SetGroupBS(Setter& setter)
	{
		auto& bs = groupBlends[setter.group];
		const bool updateBS = !ParentHigherPrio(setter.group) && (bs.targetWeight == 0.f || TransitionIsFaster(globalTime, setter.transitionLength, bs.transitionStart, bs.transitionLength));
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

	void RigAnimator::ResetSetter(Setter& setter)
	{
		for (u32 i = 0; i < MAX_TARGETS; i++)
			setter.animationIDs[i] = -1;
		setter.group = fullBodyGroup;
		setter.playbackRate = 1.f;
		setter.priority = BASE_PRIORITY;
		setter.transitionLength = 0.1f;
		setter.flag = AnimationFlag::None;
	}

	f32 RigAnimator::GetGroupWeight(u32 group)
	{
		return groups[group].weight;
		// Keep this for reference
		// auto parent = groups[group].parent;
		//const auto prio = groups[group].priority;
		//const auto weight = groups[group].weight;
		//static constexpr i16 origin = -1;
		//// Travel group tree, if a parent group has higher prio group has no influence
		//while (parent != origin)
		//	if (prio < groups[parent].priority)
		//		return 0.f;
		//	else
		//		parent = groups[parent].parent;

		//return weight;
	}
	bool RigAnimator::ParentHigherPrio(u32 group)
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

	u32 RigAnimator::GetGroupStartIdx(u32 group)
	{
		u32 idx = 0;
		for (u32 i = 0; i < group; i++)
			idx += groupClipCount[i];
		return idx;
	}

	void RigAnimator::Update(f32 dt)
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
}