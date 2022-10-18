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
			case DOG::AnimationBlendMode::normal:
				c.UpdateClip(dt);
				break;
			case DOG::AnimationBlendMode::linear:
				c.UpdateLinear(dt);
				break;
			case DOG::AnimationBlendMode::bezier:
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
			blendMode = AnimationBlendMode::normal;
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
						(gt > transitionStart &&
						(gt + dt < transitionStart + transitionLength || loop));
	}


	f32 RealAnimationComponent::AnimationClip::UpdateClipTick(const f32 dt)
	{
		normalizedTime += timeScale * dt / duration;
		// reset normalized time
		if (loop)
		{
			while (normalizedTime > 1.0f)
				normalizedTime -= 1.0f;
			while (normalizedTime < 0.0f)
				normalizedTime += 1.0f;
		}
		normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);
		currentTick = normalizedTime * totalTicks;

		return currentTick;
	};

	f32 RealAnimationComponent::AnimationClip::UpdateWeightLinear(const f32 gt, const f32 dt)
	{
		const f32 transitionTime = gt - transitionStart;
		if (transitionTime > transitionLength) // Transition is done
			currentWeight = targetWeight;
		else if (transitionTime > 0.0f) // Linear weight transition
			currentWeight = startWeight + transitionTime * (targetWeight - startWeight) / transitionLength;

		assert(currentWeight >= 0.0f);
		return currentWeight;
	}

	f32 RealAnimationComponent::AnimationClip::UpdateWeightBezier(const f32 gt, const f32 dt)
	{
		const f32 transitionTime = gt - transitionStart;
		if (transitionTime > transitionLength) // Transition is done
			currentWeight = targetWeight;
		else if (transitionTime > 0.0f) // bezier curve transition
		{
			const f32 u = transitionTime / transitionLength;
			const f32 v = 1.0f - u;
			currentWeight = startWeight * (pow(v, 3) + 3 * pow(v, 2) * u) +
							targetWeight * (3 * v * pow(u, 2) + pow(u, 3));
		}
		assert(currentWeight >= 0.0f);
		return currentWeight;
	}
	void RealAnimationComponent::AnimationClip::ResetClip()
	{
		static constexpr i32 noanimation = -1;
		
		animationID = noanimation;
		activeAnimation = false;
	}

	void RealAnimationComponent::Update(const f32 dt)
	{
		globalTime += dt;

		u32 activeClips = 0;
		u32 activatedClips = 0;
		f32 newTargetWeights[10] = { 0.f };
		f32 newTargetWeightsSum = 0.0f;
		f32 currentWeightSum = 0.0f;
		
		// update clip states and count active clips
		for (auto& c : clips)
		{
			// Keep track of added targetWeights
			if (c.BecameActive(globalTime, dt))
			{
				newTargetWeights[activatedClips++] = c.targetWeight;
				newTargetWeightsSum = newTargetWeightsSum + c.targetWeight;
			}
			c.UpdateState(globalTime, dt);
			activeClips += c.activeAnimation;
			currentWeightSum += c.activeAnimation * c.currentWeight;
		}
		
		// normalize current weights and Recalculate target Weights if new clips were activated
		if (activatedClips > 0)
		{
			for (auto& c : clips)
			{
				// recalculate target weight on previously active clips
				if (!c.BecameActive(globalTime, dt))
					c.targetWeight *= std::clamp(1.f - newTargetWeightsSum, 0.f, 1.f) * (activeClips - activatedClips);
				// normalize added targets if added weights exceed one
				else if (newTargetWeightsSum > 1.0f)
					c.targetWeight /= newTargetWeightsSum;
			}
		}
			
		// sort clips Active>group>targetWeight>currentWeight
		std::sort(clips.begin(), clips.end());

		// Update weights / tick of active clips
		for (u32 i = 0; i < activeClips; i++)
		{
			auto& c = clips[i];
			c.UpdateClipTick(dt);
			c.UpdateWeightLinear(globalTime, dt);
		}
	}
}