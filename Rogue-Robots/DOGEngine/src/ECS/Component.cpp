#include "Component.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DOG
{
	// Math functions for the TransformComponent

	TransformComponent::TransformComponent(const Vector3& position, const Vector3& rotation, const Vector3& scale) noexcept
	{
		auto t = Matrix::CreateTranslation(position);
		auto r = Matrix::CreateFromYawPitchRoll(rotation);
		auto s = Matrix::CreateScale(scale);
		worldMatrix = s * r * t;
	}

	TransformComponent& TransformComponent::SetPosition(const Vector3& position) noexcept
	{
		worldMatrix.Translation(position);
		return *this;
	}

	TransformComponent& TransformComponent::SetRotation(const Vector3& rotation) noexcept
	{
		XMVECTOR scale, rotationQuat, translation;
		XMMatrixDecompose(&scale, &rotationQuat, &translation, worldMatrix);
		worldMatrix = XMMatrixScalingFromVector(scale) *
			XMMatrixRotationRollPitchYawFromVector(rotation) *
			XMMatrixTranslationFromVector(translation);
		return *this;
	}

	TransformComponent& TransformComponent::SetRotation(const Matrix& rotationMatrix) noexcept
	{
		XMVECTOR scale, rotationQuat, translation;
		XMMatrixDecompose(&scale, &rotationQuat, &translation, worldMatrix);
		worldMatrix = XMMatrixScalingFromVector(scale) *
			(XMMATRIX) rotationMatrix *
			XMMatrixTranslationFromVector(translation);
		return *this;
	}

	TransformComponent& TransformComponent::SetScale(const Vector3& scale) noexcept
	{
		XMVECTOR xmScale, rotationQuat, translation;
		XMMatrixDecompose(&xmScale, &rotationQuat, &translation, worldMatrix);
		worldMatrix = XMMatrixScalingFromVector(scale) *
			XMMatrixRotationQuaternion(rotationQuat) *
			XMMatrixTranslationFromVector(translation);
		return *this;
	}

	Vector3 TransformComponent::GetPosition() const noexcept
	{
		return { worldMatrix(3, 0) , worldMatrix(3, 1), worldMatrix(3, 2) };
	}

	Matrix TransformComponent::GetRotation() const noexcept
	{
		XMVECTOR xmScale, rotationQuat, translation;
		XMMatrixDecompose(&xmScale, &rotationQuat, &translation, worldMatrix);
		return XMMatrixRotationQuaternion(rotationQuat);
	}

	DirectX::SimpleMath::Vector3 TransformComponent::GetScale() const noexcept
	{
		XMVECTOR xmScale, rotationQuat, translation;
		XMMatrixDecompose(&xmScale, &rotationQuat, &translation, worldMatrix);
		return xmScale;
	}

	TransformComponent& TransformComponent::RotateW(const Vector3& rotation) noexcept
	{
		XMVECTOR scaleVec, rotationQuat, translationVec;
		XMMatrixDecompose(&scaleVec, &rotationQuat, &translationVec, worldMatrix);
		XMMATRIX r = XMMatrixRotationQuaternion(rotationQuat);
		XMMATRIX s = XMMatrixScalingFromVector(scaleVec);
		XMMATRIX t = XMMatrixTranslationFromVector(translationVec);
		XMMATRIX deltaR = XMMatrixRotationRollPitchYawFromVector(rotation);
		worldMatrix = s * r * deltaR * t;
		return *this;
	}

	TransformComponent& TransformComponent::RotateW(const Matrix& rotation) noexcept
	{
		XMVECTOR scaleVec, rotationQuat, translationVec;
		XMMatrixDecompose(&scaleVec, &rotationQuat, &translationVec, worldMatrix);
		XMMATRIX r = XMMatrixRotationQuaternion(rotationQuat);
		XMMATRIX s = XMMatrixScalingFromVector(scaleVec);
		XMMATRIX t = XMMatrixTranslationFromVector(translationVec);
		XMMATRIX deltaR = rotation;
		worldMatrix = s * r * deltaR * t;
		return *this;
	}

	TransformComponent& TransformComponent::RotateL(const Vector3& rotation) noexcept
	{
		XMVECTOR scaleVec, rotationQuat, translationVec;
		XMMatrixDecompose(&scaleVec, &rotationQuat, &translationVec, worldMatrix);
		XMMATRIX r = XMMatrixRotationQuaternion(rotationQuat);
		XMMATRIX s = XMMatrixScalingFromVector(scaleVec);
		XMMATRIX t = XMMatrixTranslationFromVector(translationVec);
		XMMATRIX deltaR = XMMatrixRotationRollPitchYawFromVector(rotation);
		worldMatrix = s * deltaR * r * t;
		return *this;
	}

	TransformComponent& TransformComponent::RotateL(const Matrix& rotation) noexcept
	{
		XMVECTOR scaleVec, rotationQuat, translationVec;
		XMMatrixDecompose(&scaleVec, &rotationQuat, &translationVec, worldMatrix);
		XMMATRIX r = XMMatrixRotationQuaternion(rotationQuat);
		XMMATRIX s = XMMatrixScalingFromVector(scaleVec);
		XMMATRIX t = XMMatrixTranslationFromVector(translationVec);
		XMMATRIX deltaR = rotation;
		worldMatrix = s * deltaR * r * t;
		return *this;
	}

	// Animation update
	void AnimationComponent::AnimationClip::UpdateClip(const f32 dt) 
	{
		if (HasActiveAnimation())
		{
			normalizedTime += timeScale * dt / duration;
			if(loop)
			{
				while (normalizedTime > 1.0f)
					normalizedTime -= 1.0f;
				while (normalizedTime < 0.0f)
					normalizedTime += 1.0f;
			}
			normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);
			currentTick = normalizedTime * totalTicks;
		}
	};

	void AnimationComponent::AnimationClip::SetAnimation(const i32 id, const f32 nTicks, const f32 animDuration, const f32 startTime)
	{
		animationID = id;
		totalTicks = nTicks;
		duration = animDuration;
		normalizedTime = startTime;
	}


	void AnimationComponent::Update(const f32 dt)
	{
		globalTime += dt;
		for (auto& c : clips)
		{
			switch (c.blendMode)
			{
			case DOG::BlendMode::normal:
				c.UpdateClip(dt);
				break;
			case DOG::BlendMode::linear:
				c.UpdateLinear(dt);
				break;
			case DOG::BlendMode::bezier:
				c.UpdateBezier(dt);
				break;
			default:
				break;
			}
		}
	}

	void AnimationComponent::AnimationClip::UpdateLinear(const f32 dt)
	{
		UpdateClip(dt);
		// Linear transition between current weight to target weight of clip

		if (transitionTime <= 0.f) // apply desired weight
		{
			currentWeight = targetWeight;
			blendMode = BlendMode::normal;
		}
		else if (normalizedTime > transitionStart)
		{
			const f32 wDiff = targetWeight - currentWeight;
			const f32 tCurrent = normalizedTime - transitionStart;
			currentWeight += wDiff * tCurrent / transitionTime;
			currentWeight = std::clamp(currentWeight, 0.0f, 1.0f);
			transitionTime -= tCurrent;
			transitionStart += tCurrent;
		}
	}

	void AnimationComponent::AnimationClip::UpdateBezier(const f32 dt)
	{
		UpdateClip(dt);
		// Bezier curve transition between current weight to target weight of clip
		if (normalizedTime > transitionStart)
		{
			const f32 t = normalizedTime - transitionStart;
			const f32 u = t / (transitionTime);
			const f32 v = (1.0f - u);
			currentWeight += targetWeight * (3.f * v * u * u + std::powf(u, 3.f));
			currentWeight = std::clamp(currentWeight, 0.0f, 1.0f);
		}
	}

	// Realest one after all
	f32 RealAnimationComponent::AnimationClip::UpdateClipTick(const f32 delta)
	{
		normalizedTime += delta * timeScale / duration;
		// loop normalized time
		if (loop)
			normalizedTime = fmodf(normalizedTime, 1.0f);
		else
			normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);

		return normalizedTime * totalTicks;
	};

	void RealAnimationComponent::AnimationClip::ResetClip()
	{
		timeScale = 1.f;
		animationID = noAnimation;
		group = noGroup;
		normalizedTime = 0.f;
		activeAnimation = false;
	}

	void RealAnimationComponent::Update(const f32 dt)
	{
		globalTime += dt;

		f32 groupWeightSum[nGroups] = { 0.f };
		
		// update clip states and count active clips
		for (u8 i = 0; i < maxClips; i++)
		{
			auto& c = clips[i];
			// Keep track of clips that activated/deactivated this frame
			if (c.Activated(globalTime, dt))
			{
				if (!ReplacedClip(c))
					++clipsInGroup[c.group];
			}
			else if(c.Deactivated())
			{
				--clipsInGroup[c.group];
				c.ResetClip();
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

			clipData[i].tick = c.UpdateClipTick(transitionTime < dt ? transitionTime : dt);
			switch (c.blendMode)
			{
			case BlendMode::linear:
				clipData[i].weight = LinearBlend(transitionTime, c.transitionLength, c.startWeight, c.targetWeight, c.currentWeight);
			case BlendMode::bezier:
				clipData[i].weight = BezierBlend(transitionTime, c.transitionLength, c.startWeight, c.targetWeight, c.currentWeight);
			default:
				break;
			}
			groupWeightSum[c.group] += clipData[i].weight;
		}

		// Normalize group weights
		for (u8 i = 0; i < activeClips; i++)
		{
			if (i<clipsInGroup[groupA])
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
			std::swap(groups[inactiveBlendIdxA], groups[activeBlendIdxA]);
			auto& bsA = groups[activeBlendIdxA];
			bsA.startWeight = groupWeights[groupA];
			if (bsA.duration > 0)
				AddBlendSpecification(bsA.duration - bsA.transitionLength, bsA.transitionLength, groupA, groupWeights[groupA]);
		}
		if (groups[inactiveBlendIdxB].Activated(globalTime, dt))
		{
			std::swap(groups[inactiveBlendIdxB], groups[activeBlendIdxB]);
			auto& bsB = groups[activeBlendIdxB];
			bsB.startWeight = groupWeights[groupB];
			if (bsB.duration > 0)
				AddBlendSpecification(bsB.duration - bsB.transitionLength, bsB.transitionLength, groupB, groupWeights[groupB]);
		}

		// Set Group weights for partial body groups
		for (i32 i = 0; i < 2; i++)
		{
			const auto& group = groups[i];
			switch (group.blendMode)
			{
			case BlendMode::linear:
				groupWeights[i] = LinearBlend(globalTime - group.transitionStart, group.transitionLength, group.startWeight, group.targetWeight, groupWeights[i]);
				break;
			case BlendMode::bezier:
				groupWeights[i] = BezierBlend(globalTime - group.transitionStart, group.transitionLength, group.startWeight, group.targetWeight, groupWeights[i]);
				break;
			case BlendMode::interrupt:
				groupWeights[i] = group.targetWeight;
				break;
			default:
				break;
			}
		}
		// reset added clips for next frame
		nAddedClips = 0;
	}

	void RealAnimationComponent::AddAnimationClip(i8 id, u8 group, f32 startDelay, f32 transitionLength, f32 startWeight, f32 targetWeight, bool loop, f32 timeScale)
	{
		++nAddedClips;
		i32 clipIdx = (nAddedClips < maxClips) * (maxClips - nAddedClips);
		
		// set new clip
		auto& addedClip = clips[clipIdx];

		addedClip.animationID = id;
		addedClip.group = group;
		addedClip.transitionStart = globalTime + startDelay;
		addedClip.transitionLength = transitionLength;
		addedClip.blendMode = BlendMode::linear;
		//addedClip.blendMode = WeightBlendMode::bezier;
		addedClip.startWeight = addedClip.currentWeight = startWeight;
		addedClip.targetWeight = targetWeight;
		addedClip.timeScale = timeScale;
		addedClip.loop = loop;
	}

	void RealAnimationComponent::AnimationClip::SetAnimation(const f32 animationDuration, const f32 nTicks)
	{
		duration = animationDuration;
		totalTicks = nTicks;
	}
	bool RealAnimationComponent::AnimationClip::Activated(const f32 gt, const f32 dt) const
	{
		return animationID != noAnimation && (gt >= transitionStart) && (gt - dt <= transitionStart);
	}
	bool RealAnimationComponent::AnimationClip::Deactivated() const
	{
		return normalizedTime == 1.f && !loop;
	}
	void RealAnimationComponent::AnimationClip::UpdateState(const f32 gt)
	{
		activeAnimation = animationID != noAnimation &&
			(gt >= transitionStart && (normalizedTime < 1.0f || loop));
	}
	i32 RealAnimationComponent::ClipCount() const{
		i32 count = 0;
		for (auto& c : clips)
			count += (c.group != 3);
		return count - nAddedClips;
	}
	u8 RealAnimationComponent::GetGroupIndex(const u8 group) const
	{
		u8 idx = 0;
		for (u8 i = 0; i < group; i++)
			idx += clipsInGroup[i];
		return idx;
	}
	i32 RealAnimationComponent::ActiveClipCount() const
	{
		return clipsInGroup[0] + clipsInGroup[1] + clipsInGroup[2];
	}
	bool RealAnimationComponent::ReplacedClip(AnimationClip& clip)
	{
		bool overwriteClip = false;
		i32 idx = (clip.group > groupA) * clipsInGroup[groupA] +
			(clip.group > groupB) * clipsInGroup[groupB];
		const i32 lastIdx = idx + clipsInGroup[clip.group];
		for (idx; idx < lastIdx; ++idx)
			if (clips[idx].animationID == clip.animationID)
			{
				overwriteClip = clips[idx].loop != clip.loop;
				// Replace a current active clip with same group and animation
				if (overwriteClip)
				{
					clips[idx].startWeight = clips[idx].currentWeight;;
					clips[idx].targetWeight = clip.targetWeight;
					clips[idx].transitionStart = clip.transitionStart;

					const f32 durationLeft = (1.f - clips[idx].normalizedTime) * clips[idx].duration;
					clips[idx].transitionLength = clip.transitionLength < durationLeft || clip.loop ?
						clip.transitionLength : durationLeft;

					clips[idx].blendMode = clip.blendMode;
					clips[idx].loop = clip.loop;
					clip.ResetClip();
					return overwriteClip;
				}
			}
		return overwriteClip;
	}

	f32 RealAnimationComponent::BezierBlend(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue) const
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

	f32 RealAnimationComponent::LinearBlend(const f32 currentTime, const f32 transitionLength, const f32 startValue, const f32 targetValue, f32 currentValue) const
	{
		if (currentTime > transitionLength) // Transition is done
			currentValue = targetValue;
		else if (currentTime > 0.0f) // Linear weight transition
			currentValue = startValue + currentTime * (targetValue - startValue) / transitionLength;

		return currentValue;
	}

	void RealAnimationComponent::AddBlendSpecification(f32 startDelay, f32 transitionLength, u32 group, f32 targetWeight, f32 duration)
	{  
		// tmp group blend
		static constexpr u8 inactiveBlendIdxA = 2, inactiveBlendIdxB = 3;
		const auto idx = group == groupA ?
			inactiveBlendIdxA : inactiveBlendIdxB;

		groups[idx].blendMode = BlendMode::linear;
		groups[idx].transitionStart = globalTime + startDelay;
		groups[idx].transitionLength = transitionLength;
		groups[idx].targetWeight = targetWeight;
		groups[idx].duration = duration;
	}
}