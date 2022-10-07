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
			normalizedTime += timeScale * dt / animDuration;
			if(loop)
			{
				while (normalizedTime > 1.0f)
					normalizedTime -= 1.0f;
			}

			normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);
			currentTick = normalizedTime * animTotalTicks;
		}
	};

	void AnimationComponent::AnimationClip::SetAnimation(const i32 id, const f32 nTicks, const f32 duration, const f32 startTime)
	{
		animationID = id;
		animTotalTicks = nTicks;
		animDuration = duration;
		normalizedTime = startTime;
	}


	void AnimationComponent::Update(const f32 dt)
	{
		// Sort clips by group
		std::sort(clips.rbegin(), clips.rend());

		for (auto& c : clips)
			if (c.HasActiveAnimation())
				c.UpdateClip(dt);
	}
}