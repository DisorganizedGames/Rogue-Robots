#include "RigAnimator.h"

namespace DOG
{
	RigAnimator::RigAnimator()
	{
		static constexpr u32 nSets = 2;
		for (u32 i = 0; i < N_GROUPS; i++)
		{
			groups[i].sets[LOOPING].startClip = i * nSets * MAX_CLIPS;
			groups[i].sets[ACTION].startClip = (i * nSets + 1) * MAX_CLIPS;
		}
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
			groupClipCount[g] = UpdateGroup(groups[g], idx, dt);
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

	u32 RigAnimator::UpdateGroup(AnimationGroup& group, const u32 clipIdx, const f32 dt)
	{
		auto& looping = group.sets[LOOPING];
		auto& action = group.sets[ACTION];
		
		UpdateBlendSpec(group, dt);

		u32 count = 0;
		if(looping.weight > 0.f)
			count += UpdateClipSet(looping, clipIdx, dt, true);
		if(action.weight > 0.f)
			count += UpdateClipSet(action, clipIdx+count, dt);
	
		return count;
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

	void RigAnimator::HandleAnimationComponent(AnimationComponent& ac)
	{
		// sort added setters by group and loop status
		std::sort(ac.animSetters2.begin(), ac.animSetters2.begin() + ac.addedSetters,
			[](const Setter& a, const Setter& b) -> bool
			{
				return a.group < b.group || (a.group == b.group && a.loop && !b.loop);
			});

		for (u32 i = 0; i < ac.addedSetters; i++)
		{
			auto& setter = ac.animSetters2[i];
			auto& group = groups[setter.group];

			HandleSetter(setter, group);
		}
	}
	u32 RigAnimator::ValidateSetter(Setter& setter)
	{
		static constexpr i32 INVALID = -1;
		u32 clipCount = 0;
		f32 wSum = 0.f;
		// Get clip count and target weight sum
		for (u32 i = 0; i < MAX_TARGETS; i++)
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
	void RigAnimator::HandleSetter(Setter& setter, AnimationGroup& group)
	{
		// Normalize weights
		u32 clipCount = ValidateSetter(setter);
		// Get corresponding set
		auto& set = setter.loop ?
			group.sets[LOOPING] : group.sets[ACTION];

		// Add/modify the target
		AddTargetSet(set, setter, clipCount);
		
		// ChangeBlendSpec if Action set was added as target
		if (group.primary == LOOPING && !setter.loop)
			SetBlendSpec(group, setter);

		ResetSetter(setter);
	}

	void RigAnimator::SetBlendSpec(AnimationGroup& group, const Setter& setter)
	{
		group.primary = ACTION;
		const auto clipIdx = group.sets[ACTION].startClip;
		group.blend.duration = clips[clipIdx].duration;
		group.blend.scale = setter.playbackRate;
		group.blend.transitionStart = globalTime;
		group.blend.transitionLength = setter.transitionLength;
	}

	void RigAnimator::AddTargetSet(ClipSet& set, Setter& setter, u32 clipCount)
	{
		f32 startTime = 0.f; // an after thought
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
		set.nTargetClips = clipCount;

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

	f32 RigAnimator::LinearWeight(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue)
	{
		f32 currentValue = 0.f;
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

	void RigAnimator::UpdateBlendSpec(AnimationGroup& group, const f32 dt)
	{
		auto& bs = group.blend;
		if (group.primary == ACTION)
		{ // Action clipSets have a duration
			const f32 delta = dt * bs.scale;
			bs.duration -= delta;
			if (bs.duration <= bs.transitionLength)
			{ // Start transition back to looping clips
				const f32 diff = delta < fabsf(bs.duration) ? delta : fabsf(bs.duration);
				bs.transitionStart = globalTime-diff;
				bs.startWeight = group.sets[LOOPING].weight;
				group.primary = LOOPING;
				TransitionOutClips(group.sets[ACTION], bs.transitionStart, bs.transitionLength);
			}
		}
		// Time since transition started
		const f32 currentTime = globalTime - bs.transitionStart;
		// Update weight of the target set
		f32& primaryWeight = group.sets[group.primary].weight;
		primaryWeight = LinearWeight(currentTime, bs.transitionLength, bs.startWeight, 1.0f);
		// Update weight of the secondary set
		group.sets[group.primary ^ 1].weight = 1.0f - primaryWeight;
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