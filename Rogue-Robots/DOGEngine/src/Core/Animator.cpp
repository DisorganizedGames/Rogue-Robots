#include "Animator.h"
#include "Tracy/Tracy.hpp"

namespace DOG
{
	// Realest one after all
	f32 Animator::AnimationClip::UpdateClipTick(const f32 delta)
	{
		normalizedTime += delta * timeScale / duration;
		// loop normalized time
		if (loop)
			normalizedTime = fmodf(normalizedTime, 1.0f);
		else
			normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);
	
		return normalizedTime * totalTicks;
	};
	
	void Animator::ResetClip(AnimationClip& clip)
	{
		if (clip.group != noGroup && clipsInGroup[clip.group] != 0)
			--clipsInGroup[clip.group];
		clip.timeScale = 1.f;
		clip.animationID = NO_ANIMATION;
		clip.group = noGroup;
		clip.normalizedTime = 0.f;
		clip.activeAnimation = false;
	}
	
	void Animator::Update(const f32 dt)
	{
		ZoneScopedN("animatorUpdate");
		globalTime += dt;
	
		f32 groupWeightSum[nGroups] = { 0.f };
	
		// update clip states and count active clips
		for (u8 i = 0; i < maxClips; i++)
		{
			auto& c = clips[i];
			// Keep track of clips that activated/deactivated this frame
			if (c.Activated(globalTime, dt))
			{
				//if (!ReplacedClip(c, i))
				// hack
				if (clipsInGroup[groupC] > 1 && c.group == groupC)
				{
					ResetClip(c);
				}
				else
				{
					++clipsInGroup[c.group];
				}
			}
			else if (c.Deactivated())
			{
				ResetClip(c);
			}
			c.UpdateState(globalTime);
		}
		
		
		// sort clips Active-group-targetWeight-currentWeight
		std::sort(clips.begin(), clips.end());
	
		u32 activeClips = ActiveClipCount();
		// Update weights / tick of active clips
		for (u32 i = 0; i < activeClips; i++)
		{
			auto& c = clips[i];
			clipData[i].aID = c.animationID;
			const auto transitionTime = globalTime - c.transitionStart;
	
			clipData[i].tick = c.UpdateClipTick(transitionTime < dt ? globalTime - c.transitionStart : dt);
			clipData[i].weight = LinearBlend(transitionTime, c.transitionLength, c.startWeight, c.targetWeight, c.currentWeight);
			
			groupWeightSum[c.group] += clipData[i].weight;
		}
	
		// Normalize group weights
		for (u8 i = 0; i < activeClips; i++)
		{
			if (i < clipsInGroup[groupA])
				clipData[i].weight /= groupWeightSum[groupA];
			else if (i < clipsInGroup[groupA] + clipsInGroup[groupB])
				clipData[i].weight /= groupWeightSum[groupB];
			else
				clipData[i].weight /= groupWeightSum[groupC];
		}
		// tmp sad blend solution
		static constexpr u8 activeBlendIdxA = 0, activeBlendIdxB = 1;
		static constexpr u8 inactiveBlendIdxA = 2, inactiveBlendIdxB = 3;
		if (groups[inactiveBlendIdxA].Activated(globalTime, dt))
		{
			f32 retWeight = groups[activeBlendIdxA].startWeight;
			std::swap(groups[inactiveBlendIdxA], groups[activeBlendIdxA]);
			auto& bsA = groups[activeBlendIdxA];
			bsA.startWeight = groupWeights[groupA];
			if (bsA.duration > 0)
				AddBlendSpecification(std::clamp(bsA.duration - bsA.transitionLength, 0.f, 1.f), bsA.transitionLength, groupA, retWeight);
		}
		if (groups[inactiveBlendIdxB].Activated(globalTime, dt))
		{
			f32 retWeight = groups[activeBlendIdxB].startWeight;
			std::swap(groups[inactiveBlendIdxB], groups[activeBlendIdxB]);
			auto& bsB = groups[activeBlendIdxB];
			bsB.startWeight = groupWeights[groupB];
			if (bsB.duration > 0)
				AddBlendSpecification(std::clamp(bsB.duration - bsB.transitionLength, 0.f, 1.f), bsB.transitionLength, groupB, retWeight);
		}
	
		// Set Group weights for partial body groups
		for (i32 i = 0; i < 2; i++)
		{
			const auto& group = groups[i];
			groupWeights[i] = LinearBlend(globalTime - group.transitionStart, group.transitionLength, group.startWeight, group.targetWeight, groupWeights[i]);

		}
		// make sure weight is zeroed if no clips tmp solution
		if(clipsInGroup[groupB] == 0)
			groupWeights[groupB] = 0.f;
		if (clipsInGroup[groupA] == 0)
			groupWeights[groupA] = 0.f;
		// reset added clips for next frame
		nAddedClips = 0;
	}

	bool Animator::OverwriteClip(AnimationClip& clip)
	{
		bool overwriteClip = false;
		i32 idx = (clip.group > groupA) * clipsInGroup[groupA] +
			(clip.group > groupB) * clipsInGroup[groupB];
		const i32 lastIdx = idx + clipsInGroup[clip.group];
		for (u32 i = 0; i < lastIdx; i++)
		{
			if (clip.animationID == clips[i].animationID && clip.group == clips[i].group)
			{
				clips[i].startWeight = clip.startWeight;
				clips[i].targetWeight = clip.targetWeight;
				clips[i].transitionStart = globalTime + clip.transitionStart;
				clips[i].transitionLength = clip.transitionLength;
				clips[i].normalizedTime = 0.f;
				clips[i].loop = clip.loop;
				clips[i].timeScale = clip.timeScale;
				return true;
			}
		}
		return false;
	}

	void Animator::AddAnimationClip(i8 id, f32 duration, f32 ticks, u8 group, f32 transitionLength, f32 startWeight, f32 targetWeight, bool loop, f32 timeScale, f32 startDelay)
	{
		i32 clipIdx = ClipIdx(group, id);
		ResetClip(clips[clipIdx]);

		// set new clip
		auto& addedClip = clips[clipIdx];
	
		addedClip.animationID = id;
		addedClip.group = group;
		addedClip.transitionStart = globalTime + startDelay;
		addedClip.transitionLength = transitionLength;
		//addedClip.blendMode = WeightBlendMode::bezier;
		addedClip.startWeight = addedClip.currentWeight = startWeight;
		addedClip.targetWeight = targetWeight;
		addedClip.timeScale = timeScale;
		addedClip.loop = loop;
		addedClip.duration = duration;
		addedClip.totalTicks = ticks;
	}

	u32 Animator::ClipIdx(const u8 group, const u8 animationID)
	{
		//for (idx; idx < lastIdx; ++idx)
		for (u32 idx = 0; idx < maxClips; ++idx)
		{
			if (clips[idx].animationID == animationID && clips[idx].group == group)
			{
				return idx;
			}
		}
		++nAddedClips;
		i32 clipIdx = maxClips - nAddedClips;
		if (nAddedClips >= maxClips)
		{
			clipIdx = maxClips - 1;
		}
		return clipIdx;
	}
	
	void Animator::AnimationClip::SetAnimation(const f32 animationDuration, const f32 nTicks)
	{
		duration = animationDuration;
		totalTicks = nTicks;
	}
	bool Animator::AnimationClip::Activated(const f32 gt, const f32 dt) const
	{
		return animationID != noAnimation && (gt >= transitionStart) && (gt - dt <= transitionStart);
	}
	bool Animator::AnimationClip::Deactivated() const
	{
		return normalizedTime == 1.f && !loop;
	}
	void Animator::AnimationClip::UpdateState(const f32 gt)
	{
		activeAnimation = animationID != NO_ANIMATION &&
			(gt >= transitionStart && (normalizedTime < 1.0f || loop));
	}
	i32 Animator::ClipCount() const {
		i32 count = 0;
		for (auto& c : clips)
			count += (c.group != 3);
		return count - nAddedClips;
	}
	u8 Animator::GetGroupIndex(const u8 group) const
	{
		u8 idx = 0;
		for (u8 i = 0; i < group; i++)
			idx += clipsInGroup[i];
		return idx;
	}
	i32 Animator::ActiveClipCount() const
	{
		return clipsInGroup[0] + clipsInGroup[1] + clipsInGroup[2];
	}
	bool Animator::ReplacedClip(AnimationClip& clip, u32 cidx)
	{
		bool overwriteClip = false;
		i32 idx = (clip.group > groupA) * clipsInGroup[groupA] +
			(clip.group > groupB) * clipsInGroup[groupB];
		const i32 lastIdx = idx + clipsInGroup[clip.group];
		for (idx; idx < lastIdx; ++idx)
			if (clips[idx].animationID == clip.animationID)
			{
				overwriteClip = true;
				// Replace a current active clip with same group and animation
				if (overwriteClip)
				{
					clips[idx].startWeight = clip.startWeight;
					clips[idx].targetWeight = clip.targetWeight;
					clips[idx].transitionStart = clip.transitionStart;
					clips[idx].transitionLength = clip.transitionLength;
					clips[idx].normalizedTime = 0.f;
					clips[idx].loop = clip.loop;
					if(idx != cidx)
						ResetClip(clip);
					return overwriteClip;
				}
			}
		return overwriteClip;
	}
	
	f32 Animator::BezierBlend(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue) const
	{
		if (currentTime >= transitionLength) // Transition is done
			currentValue = targetValue;
		else if (currentTime > 0.0f) // bezier curve transition
		{
			const f32 u = currentTime / transitionLength;
			const f32 v = 1.0f - u;
			currentValue = startValue * (powf(v, 3) + 3 * powf(v, 2) * u) +
				targetValue * (3 * v * powf(u, 2) + powf(u, 3));
		}
		return currentValue;
	}
	
	f32 Animator::LinearBlend(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue) const
	{
		if (currentTime > transitionLength) // Transition is done
			currentValue = targetValue;
		else if (currentTime > 0.0f) // Linear weight transition
			currentValue = startValue + currentTime * (targetValue - startValue) / transitionLength;
	
		return currentValue;
	}
	
	void Animator::AddBlendSpecification(f32 startDelay, f32 transitionLength, u32 group, f32 targetWeight, f32 duration)
	{
		// tmp group blend
		static constexpr u8 inactiveBlendIdxA = 2, inactiveBlendIdxB = 3;
		const auto idx = group == groupA ?
			inactiveBlendIdxA : inactiveBlendIdxB;
	
		groups[idx].transitionStart = globalTime + startDelay;
		groups[idx].transitionLength = transitionLength;
		groups[idx].targetWeight = targetWeight;
		groups[idx].duration = duration;
	}

	bool Animator::GroupBlendSpec::Activated(const f32 gt, const f32 dt) const {
		return (gt >= transitionStart) && (gt - dt <= transitionStart);
	}
}
