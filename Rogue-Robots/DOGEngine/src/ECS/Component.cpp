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
			case DOG::WeightBlendMode::normal:
				c.UpdateClip(dt);
				break;
			case DOG::WeightBlendMode::linear:
				c.UpdateLinear(dt);
				break;
			case DOG::WeightBlendMode::bezier:
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
			blendMode = WeightBlendMode::normal;
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

	// Animation update
	void RealAnimationComponent::AnimationClip::UpdateState(const f32 gt, const f32 dt)
	{
		static constexpr i32 noAnimation = -1;
		activeAnimation = animationID != noAnimation &&
						(gt > transitionStart && (gt < (transitionStart + transitionLength) || loop));
		/*Based on normalized time instead??
		activeAnimation = animationID != noAnimation &&
							(gt > transitionStart && (normalizedTime < 1.0f || loop));
		*/
	}


	f32 RealAnimationComponent::AnimationClip::UpdateClipTick(const f32 gt)
	{
		normalizedTime = timeScale * (gt-transitionStart) / duration;
		// reset normalized time
		if (loop)
		{
			//normalizedTime = fmodf(normalizedTime, 1.0f);
			while (normalizedTime > 1.0f)
				normalizedTime -= 1.0f;
			while (normalizedTime < 0.0f)
				normalizedTime += 1.0f;
		}
		normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);
		currentTick = normalizedTime * totalTicks;

		return currentTick;
	};

	f32 RealAnimationComponent::AnimationClip::UpdateWeightLinear(const f32 gt)
	{
		const f32 transitionTime = gt - transitionStart;
		if (transitionTime > transitionLength) // Transition is done
			currentWeight = targetWeight;
		else if (transitionTime > 0.0f) // Linear weight transition
			currentWeight = startWeight + transitionTime * (targetWeight - startWeight) / transitionLength;

		assert(currentWeight >= 0.0f);
		return currentWeight;
	}

	f32 RealAnimationComponent::AnimationClip::UpdateWeightBezier(const f32 gt)
	{
		const f32 transitionTime = gt - transitionStart;
		if (transitionTime > transitionLength) // Transition is done
			currentWeight = targetWeight;
		else if (transitionTime > 0.0f) // bezier curve transition
		{
			const f32 u = transitionTime / transitionLength;
			const f32 v = 1.0f - u;
			currentWeight = startWeight * (powf(v, 3) + 3 * powf(v, 2) * u) +
							targetWeight * (3 * v * powf(u, 2) + powf(u, 3));
		}
		assert(currentWeight >= 0.0f);
		return currentWeight;
	}
	void RealAnimationComponent::AnimationClip::ResetClip()
	{
		static constexpr i32 noanimation = -1;
		timeScale = 1.f;
		animationID = noanimation;
		group = 3;
		activeAnimation = false;
	}

	void RealAnimationComponent::Update(const f32 dt)
	{
		globalTime += dt;

		f32 groupWeightSum[3] = { 0.f };
		
		// update clip states and count active clips
		for (auto& c : clips)
		{
			// Keep track of clips that activated this frame
			if (c.BecameActive(globalTime, dt))
			{
				++animsPerGroup[c.group];
			}
			else if(c.Deactivated(globalTime, dt))
			{
				--animsPerGroup[c.group];
				c.ResetClip();
			}

			c.UpdateState(globalTime, dt);
		}
			
		// sort clips Active>group>targetWeight>currentWeight
		std::sort(clips.begin(), clips.end());

		u32 activeClips = animsPerGroup[0] + animsPerGroup[1] + animsPerGroup[2];
		// Update weights / tick of active clips
		for (u32 i = 0; i < activeClips; i++)
		{
			auto& c = clips[i];
			c.UpdateClipTick(globalTime);
			c.UpdateWeightLinear(globalTime);
			groupWeightSum[c.group] += c.currentWeight;
		}

		// normalize group weights
		for (u32 i = 0; i < activeClips; i++)
			if (groupWeightSum[clips[i].group] > 0.0f)
				clips[i].currentWeight /= groupWeightSum[clips[i].group];

		// reset added clips for next frame
		nAddedClips = 0;
	}

	void RealAnimationComponent::AddAnimationClip(i32 id, u32 group, f32 startDelay, f32 transitionLength, f32 startWeight, f32 targetWeight, bool loop, f32 timeScale)
	{
		//if (ActiveClipCount() == maxClips)
		//{
		//	//remove clip ere
		//}
		
		// set new clip
		auto& addedClip = clips[clipCount()];
		addedClip.animationID = id;
		addedClip.group = group;
		addedClip.transitionStart = globalTime + startDelay;
		addedClip.transitionLength = transitionLength;
		addedClip.blendMode = WeightBlendMode::linear;
		addedClip.startWeight = startWeight;
		addedClip.targetWeight = targetWeight;
		addedClip.timeScale = timeScale;
		addedClip.loop = loop;
		++nAddedClips;
	}

	void RealAnimationComponent::AnimationClip::SetAnimation(const f32 animationDuration, const f32 nTicks)
	{
		duration = animationDuration;
		totalTicks = nTicks;
	}
}