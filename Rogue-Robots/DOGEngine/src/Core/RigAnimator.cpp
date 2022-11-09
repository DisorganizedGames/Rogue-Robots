#include "RigAnimator.h"

namespace DOG
{
	RigAnimator::RigAnimator()
	{
		static constexpr u32 nSets = 2;
		for (u32 i = 0; i < N_GROUPS; i++)
		{
			groups[i].sets[LOOPING].startClip = static_cast<u8>(i * nSets * MAX_CLIPS);
			groups[i].sets[ACTION].startClip = static_cast<u8>((i * nSets + 1) * MAX_CLIPS);
			// tmp
			groups[i].parent = i == 0 ? -1 : 0;
		}
	}

	bool BlendFinished(const BlendSpecification& bs, const f32 dt)
	{
		return (bs.durationLeft > 0.f && bs.durationLeft < dt);
	}
	std::pair<u32, u32>	GetStartStop(const ClipSet& set, bool target)
	{
		return { set.startClip, target ? set.startClip + set.nTargetClips : set.startClip + set.nTotalClips };
	}
	void RigAnimator::Update(const f32 dt)
	{
		globalTime += dt;
		u32 idx = 0;
		for (u32 g = 0; g < N_GROUPS; ++g)
		{
			if (BlendFinished(groups[g].blend, dt))
				groups[g].priority = BASE_PRIORITY; // Reset priority

			groupClipCount[g] = UpdateGroup(g, idx, dt);
			idx += groupClipCount[g];
		}
	}

	u32 RigAnimator::GetGroupStartIdx(u32 group)
	{
		u32 idx = 0;
		for (u32 i = 0; i < group; i++)
			idx += groupClipCount[i];
		return idx;
	}

	u32 RigAnimator::UpdateGroup(u32 groupIdx, const u32 clipIdx, const f32 dt)
	{
		auto& group = groups[groupIdx];
		auto& looping = group.sets[LOOPING];
		auto& action = group.sets[ACTION];
		
		if (BlendFinished(group.blend, dt))
		{
			const auto tlen = group.blend.transitionLength;
			TransitionOutClips(action, globalTime, tlen);

			if (!looping.nTargetClips)
			{ // If no looping state to return to also transition out group
				groupBlends[groupIdx].transitionLength = tlen;
				ResetBlendSpecification(groupBlends[groupIdx], 0.f, group.weight, 0.f);
			}
		}
		
		// Update group weight
		group.weight = UpdateBlendSpecification(groupBlends[groupIdx], dt, group.weight);

		u32 clipCount = 0;
		// Update sets and set Weights
		looping.weight = UpdateBlendSpecification(group.blend, dt, looping.weight);
		if(looping.weight > 0.f)
			clipCount += UpdateClipSet(looping, clipIdx, dt, true);

		action.weight = 1.f - looping.weight;
		if(action.weight > 0.f)
			clipCount += UpdateClipSet(action, clipIdx+clipCount, dt);

		return clipCount;
	}

	u32 RigAnimator::UpdateClipSet(ClipSet& set, const u32 clipIdx, const f32 dt, bool matching)
	{
		f32 targetWeightSum = 0.f, othersWeightSum = 0.f;
		u32 nClips = set.nTotalClips, idx = set.startClip, count = 0;
		
		for (u32 i = 0; i < nClips; ++i)
		{
			auto& c = clips[idx];
			const f32 transitionTime = globalTime - c.transitionStart;
			// if matching, set all clips to same normalized time as first clip in set
			f32 nt = matching ?
				ClipNormalizedTime(clips[set.startClip], transitionTime < dt ? transitionTime : dt, clips[set.startClip].loop) :
				ClipNormalizedTime(c, transitionTime < dt ? transitionTime : dt, c.loop);

			const auto id = c.aID;
			const f32 tick = ClipTick(c, nt);
			const f32 weight = c.currentWeight = LinearWeight(transitionTime, c.transitionLength, c.startWeight, c.targetWeight);
			if (i < set.nTargetClips || weight > 0.f)
			{ // Clip contributes to pose, store pose data
				const auto cIdx = clipIdx + count;
				clipData[cIdx].aID = id;
				clipData[cIdx].tick = tick;
				clipData[cIdx].weight = weight * set.weight;
				i < set.nTargetClips ?
					targetWeightSum += weight * set.weight :
					othersWeightSum += weight * set.weight;
				++count, ++idx;
			}
			else
			{ // Clip is deprecated, remove it
				std::swap(clips[idx], clips[--set.nTotalClips]);
			}
		}

		// Normalize weight data
		if (othersWeightSum > 0.0f)
		{
			const auto factor = (1.f - targetWeightSum) / othersWeightSum;
			idx = clipIdx + set.nTargetClips;
			for (u32 i = set.nTargetClips; i < set.nTotalClips; ++i, ++idx)
				clipData[idx].weight *= factor;
		}
		else if(targetWeightSum < 1.f && targetWeightSum > 0.f)
		{
			idx = clipIdx;
			for (u32 i = 0; i < set.nTargetClips; ++i, ++idx)
				clipData[idx].weight /= targetWeightSum;
		}
		// Total weights should match set weight
		idx = clipIdx;
		for (u32 i = 0; i < set.nTotalClips; ++i, ++idx)
			clipData[idx].weight *= set.weight;
		
		return count;
	}

	f32 RigAnimator::GetGroupWeight(u32 group)
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

	void RigAnimator::ProcessAnimationComponent(AnimationComponent& ac)
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

			ProcessSetter(setter, i);
			ResetSetter(setter);
		}
		ac.addedSetters = 0;
	}

	u32 RigAnimator::PreProcessSetter(Setter& setter)
	{
		static constexpr i32 INVALID = -1;
		u32 clipCount = 0;
		f32 wSum = 0.f;
		// Get clip count and target weight sum
		for (u32 i = 0; i < MAX_TARGETS; ++i)
		{
			if (setter.animationIDs[i] == INVALID)
				break;

			++clipCount;
			// targetweights must be larger than 0
			setter.targetWeights[i] = std::clamp(setter.targetWeights[i], 0.01f, 1.f);
			wSum += setter.targetWeights[i];
		}
		// Normalize target weights
		for (u32 i = 0; i < clipCount; i++)
			setter.targetWeights[i] /= wSum;

		return clipCount;
	}

	void RigAnimator::ProcessSetter(Setter& setter, u32 groupIdx)
	{
		auto& group = groups[groupIdx];
		// Normalize weights and get number of clips in setter
		u32 clipCount = PreProcessSetter(setter);

		if (!clipCount || setter.priority < group.priority)
			return;

		// Update priority
		group.priority = static_cast<u32>(setter.priority);

		// Get corresponding set
		auto& set = setter.loop ?
			group.sets[LOOPING] : group.sets[ACTION];

		// Set group blend if previously empty group
		if (!groupClipCount[groupIdx])
			ResetBlendSpecification(groupBlends[groupIdx], 0.f, 0.f, 1.f);

		// Add/modify the target
		AddTargetSet(set, setter, clipCount);
		
		// ChangeBlendSpec if Action set was added as target
		if (!setter.loop)
			SetReturningBlendSpec(group.blend, setter.transitionLength, 0.f, clips[group.sets[ACTION].startClip].duration);
	}

	void RigAnimator::AddTargetSet(ClipSet& set, Setter& setter, u32 clipCount)
	{
		// starting normalized time of added clips
		f32 startTime = 0.f;
		// Transition out old target clips
		u32 idx = set.startClip;
		for (u32 i = 0; i < set.nTargetClips; ++i, ++idx)
		{
			auto& c = clips[idx];
			c.startWeight = c.currentWeight * set.weight; // this is poop
			c.targetWeight = 0.f;
			c.transitionStart = globalTime;
			c.transitionLength = setter.transitionLength;

			if (i == 0 && setter.loop) // should make flag 'matching' instead
				startTime = c.normalizedTime;
		}

		// Set new target clips
		set.nTargetClips = static_cast<u8>(clipCount);

		static constexpr i32 NOT_FOUND = -1;
		for (u32 i = 0; i < clipCount; ++i)
		{
			auto clip = GetClipIndex(set, setter.animationIDs[i]);

			if (NOT_FOUND == clip)
			{ // Clip not part of set, add it
				AddClip(set, setter, i, startTime);
			}
			else
			{ // Clip exists, modify it
				ModifyClip(clips[clip], setter, i);
				std::swap(clips[set.startClip+i], clips[clip]);
			}
		}
	}

	void RigAnimator::AddClip(ClipSet& set, Setter& setter, u32 setIdx, f32 startTime)
	{
		// Add clip to back
		const auto clipIdx = set.nTotalClips < MAX_CLIPS ? set.startClip + set.nTotalClips : set.startClip + MAX_CLIPS - 1u;
		auto& clip = clips[clipIdx];
		clip.aID = setter.animationIDs[setIdx]; // this is poop
		clip.duration = rigData->animations[clip.aID].duration;
		clip.totalTicks = rigData->animations[clip.aID].ticks;
		clip.currentWeight = 0.f;
		clip.loop = setter.loop;
		clip.normalizedTime = startTime;
		clip.startWeight = 0.f;
		clip.targetWeight = setter.targetWeights[setIdx];
		clip.transitionLength = setter.transitionLength;
		clip.transitionStart = globalTime;
		clip.timeScale = setter.playbackRate;
		// Swap clip to desired position
		std::swap(clips[set.startClip + setIdx], clips[clipIdx]);
		++set.nTotalClips;
		set.nTotalClips = set.nTotalClips > MAX_CLIPS ? MAX_CLIPS : set.nTotalClips;
	}

	void RigAnimator::ModifyClip(Clip& clip, Setter& setter, u32 setIdx)
	{
		// Retain animation data
		// Set new weight/transition data
		clip.targetWeight = setter.targetWeights[setIdx];
		clip.startWeight = clip.currentWeight; // this is poop
		clip.transitionStart = globalTime;
		clip.timeScale = setter.playbackRate;
		clip.transitionLength = setter.transitionLength;
	}

	i32 RigAnimator::GetClipIndex(const ClipSet& set, const i32 animationID)
	{
		// Returns clip index if animation is present in the set
		static constexpr i32 NOT_FOUND = -1;
		u32 idx = set.startClip;
		for (u32 i = 0; i < set.nTotalClips; ++i, ++idx)
			if (clips[idx].aID == animationID)
				return idx;
		return NOT_FOUND;
	}

	void RigAnimator::SetClip(Setter& setter, u32 setIdx, Clip& clip)
	{
		// Set Animation data
		clip.aID = setter.animationIDs[setIdx]; // this is poop
		clip.duration = rigData->animations[clip.aID].duration;
		clip.totalTicks = rigData->animations[clip.aID].ticks;
		// Set clip data
		clip.currentWeight = 0.f; // this is poop
		clip.loop = setter.loop;
		clip.normalizedTime = 0.f;
		clip.startWeight = 0.f;
		clip.targetWeight = setter.targetWeights[setIdx];
		clip.transitionLength = setter.transitionLength;
		clip.transitionStart = globalTime;
		clip.timeScale = setter.playbackRate;
	}

	f32 RigAnimator::ClipNormalizedTime(Clip& c, const f32 delta, bool loop)
	{
		c.normalizedTime += delta * c.timeScale / c.duration;

		c.normalizedTime = loop ?
			fmodf(c.normalizedTime, 1.0f) :
			std::clamp(c.normalizedTime, 0.0f, 1.0f);

		return c.normalizedTime;
	};

	f32 RigAnimator::ClipTick(const Clip& c, const f32 nt)
	{
		return nt * c.totalTicks;
	}

	f32 RigAnimator::LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue)
	{
		if (currentTime >= transitionLength) // Transition is done
			currentValue = targetValue;
		else if (currentTime > 0.0f) // Linear
			currentValue = startValue + currentTime * (targetValue - startValue) / transitionLength;

		return currentValue;
	}

	f32 RigAnimator::BezierWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue)
	{
		f32 currentValue = 0.f;
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

	void RigAnimator::SetReturningBlendSpec(BlendSpecification& bs, const f32 transitionLen, const f32 target, const f32 duration)
	{
		bs.durationLeft = duration - transitionLen;
		bs.transitionStart = globalTime;
		bs.transitionLength = transitionLen;
		bs.targetValue = target;
	}

	void RigAnimator::ResetBlendSpecification(BlendSpecification& bs, const f32 timeDelta, const f32 startValue, const f32 targetValue)
	{
		bs.durationLeft = 0.f;
		bs.transitionStart = globalTime + timeDelta;
		bs.startWeight = startValue;
		bs.targetValue = targetValue;
	}

	f32 RigAnimator::UpdateBlendSpecification(BlendSpecification& bs, const f32 dt, const f32 currentValue)
	{
		if (bs.durationLeft > 0.f)
		{
			bs.durationLeft -= dt;
			if (bs.durationLeft <= 0.f)
			{ // Start return transition
				ResetBlendSpecification(bs, bs.durationLeft, currentValue);
			}
		}

		const auto currentTime = globalTime - bs.transitionStart;
		return LinearWeight(currentTime, bs.transitionLength, bs.startWeight, bs.targetValue, currentValue);
	}

	void RigAnimator::TransitionOutClips(ClipSet& set, const f32 transitionStart, const f32 transitionLen)
	{
		for (u32 i = set.startClip; i < set.nTargetClips; i++)
		{
			auto& c = clips[i];
			c.startWeight = c.currentWeight; // this is poop
			c.targetWeight = 0.f;
			c.transitionStart = transitionStart;
			c.transitionLength = transitionLen;
		}
	}

	void RigAnimator::ResetSetter(Setter& setter)
	{
		for (u32 i = 0; i < MAX_TARGETS; i++)
			setter.animationIDs[i] = -1;
	}
}