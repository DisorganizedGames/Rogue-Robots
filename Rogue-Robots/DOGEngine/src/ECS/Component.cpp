#include "Component.h"
#include "../Scripting/LuaTable.h"
#include "../Scripting/LuaMain.h"
#include "../Core/AssetManager.h"

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

	TransformComponent& TransformComponent::RotateForwardTo(const Vector3& target) noexcept
	{
		Vector3 normTarget = XMVector3Normalize(target);
		Vector3 forward = GetForward();
		auto angleBetween = acos(forward.Dot(normTarget));

		XMVECTOR scaleVec, rotationQuat, translationVec;
		XMMatrixDecompose(&scaleVec, &rotationQuat, &translationVec, worldMatrix);

		auto rotation = Matrix::CreateFromAxisAngle(forward.Cross(normTarget), angleBetween);

		XMMATRIX r = XMMatrixRotationQuaternion(rotationQuat);
		XMMATRIX s = XMMatrixScalingFromVector(scaleVec);
		XMMATRIX t = XMMatrixTranslationFromVector(translationVec);
		XMMATRIX deltaR = rotation;

		worldMatrix = s * deltaR * r * t;

		return *this;
	}
	void AnimationComponent::SimpleAdd(i8 animationId, AnimationFlag flags, u32 priority)
	{
		if (addedSetters < MAX_SETTERS)
		{
			auto& setter = animSetters[addedSetters++];
			setter.animationIDs[0] = animationId;
			setter.group = 0;
			setter.targetWeights[0] = 1.f;
			setter.playbackRate = 1.f;
			setter.priority = static_cast<u8>(priority);
			setter.flag = flags | AnimationFlag::SimpleAdd;
			setter.transitionLength = 0.5f;
		}
	}

	BoundingBoxComponent::BoundingBoxComponent(DirectX::SimpleMath::Vector3 center, DirectX::SimpleMath::Vector3 extents)
	{
		aabb.Center = DirectX::XMFLOAT3{ center.x, center.y, center.z };
		aabb.Extents = DirectX::XMFLOAT3{ extents.x, extents.y, extents.z };
	}

	Vector3 BoundingBoxComponent::Center()
	{
		return Vector3(aabb.Center);
	}

	Vector3 BoundingBoxComponent::Extents()
	{
		return Vector3(aabb.Extents);
	}

	void BoundingBoxComponent::Center(Vector3 c)
	{
		aabb.Center = c;
	}

	void BoundingBoxComponent::Extents(Vector3 ext)
	{
		aabb.Extents = ext;
	}

	void ParticleSystemFromFile(entity e, const std::filesystem::path& path)
	{
		LuaMain::GetScriptManager()->RunLuaFile(path.string());
		auto system = LuaMain::GetGlobal()->GetTable("system");
		auto startColor = system.GetTableFromTable("startColor");
		auto endColor = system.GetTableFromTable("endColor");
		auto spawn = system.GetTableFromTable("spawn");
		auto behavior = system.GetTableFromTable("behavior");
		
		auto texturePath = system.GetStringFromTable("texture");

		ParticleEmitterComponent emitter = {
			.spawnRate = (f32)system.GetDoubleFromTable("rate"),
			.particleSize = (f32)system.GetDoubleFromTable("size"),
			.particleLifetime = (f32)system.GetDoubleFromTable("lifetime"),
			.textureSegmentsX = (u32)system.GetIntFromTable("textureSegmentsX"),
			.textureSegmentsY = (u32)system.GetIntFromTable("textureSegmentsY"),
			.startColor = { (f32)startColor.GetDoubleFromTable("r"), (f32)startColor.GetDoubleFromTable("g"), (f32)startColor.GetDoubleFromTable("b"), (f32)startColor.GetDoubleFromTable("a") },
			.endColor = { (f32)endColor.GetDoubleFromTable("r"), (f32)endColor.GetDoubleFromTable("g"), (f32)endColor.GetDoubleFromTable("b"), (f32)endColor.GetDoubleFromTable("a") },
		};
		if (!texturePath.empty())
		{
			auto textureAsset = AssetManager::Get().LoadTexture(system.GetStringFromTable("texture"), AssetLoadFlag::GPUMemory);
			emitter.textureHandle = AssetManager::Get().GetAsset<TextureAsset>(textureAsset)->textureViewRawHandle;
		}
		EntityManager::Get().AddComponent<ParticleEmitterComponent>(e) = emitter;

		auto spawnType = spawn.GetStringFromTable("type");
		if (spawnType == "cone") { 
			EntityManager::Get().AddComponent<ConeSpawnComponent>(e) = {
				.angle = (f32)spawn.GetDoubleFromTable("angle"),
				.speed = (f32)spawn.GetDoubleFromTable("speed")
			};
		}
		else if (spawnType == "cylinder") {
			EntityManager::Get().AddComponent<CylinderSpawnComponent>(e) = {
				.radius = (f32)spawn.GetDoubleFromTable("radius"),
				.height = (f32)spawn.GetDoubleFromTable("height")
			};
		}
		else if (spawnType == "box") {
			EntityManager::Get().AddComponent<BoxSpawnComponent>(e) = {
				.x = (f32)spawn.GetDoubleFromTable("x"),
				.y = (f32)spawn.GetDoubleFromTable("y"),
				.z = (f32)spawn.GetDoubleFromTable("z")
			};
		}

		auto behaviorType = behavior.GetStringFromTable("type");
		if (behaviorType == "gravity")
		{
			EntityManager::Get().AddComponent<GravityBehaviorComponent>(e) = {
				.gravity = (f32)behavior.GetDoubleFromTable("g"),
			};
		}
		else if (behaviorType == "noGravity")
		{
			EntityManager::Get().AddComponent<NoGravityBehaviorComponent>(e);
		}
		else if (behaviorType == "gravityPoint")
		{
			EntityManager::Get().AddComponent<GravityPointBehaviorComponent>(e) = {
				.point = { (f32)behavior.GetDoubleFromTable("x"), (f32)behavior.GetDoubleFromTable("y"), (f32)behavior.GetDoubleFromTable("z") }
			};
		}
		else if (behaviorType == "gravityDirection")
		{
			EntityManager::Get().AddComponent<GravityDirectionBehaviorComponent>(e) = {
				.direction = { (f32)behavior.GetDoubleFromTable("x"), (f32)behavior.GetDoubleFromTable("y"), (f32)behavior.GetDoubleFromTable("z") }
			};
		}
		else if (behaviorType == "constVelocity")
		{
			EntityManager::Get().AddComponent<ConstVelocityBehaviorComponent>(e) = {
				.velocity = { (f32)behavior.GetDoubleFromTable("x"), (f32)behavior.GetDoubleFromTable("y"), (f32)behavior.GetDoubleFromTable("z") }
			};
		}
	}
}

