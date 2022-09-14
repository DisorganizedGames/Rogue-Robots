#pragma once
namespace DOG
{
	struct ComponentBase
	{
		ComponentBase() noexcept = default;
	protected:
		[[nodiscard]] static const u32 GetID() noexcept;
	};

	class EntityManager;
	template<typename T>
	struct Component : public ComponentBase
	{
		friend EntityManager;
		Component() noexcept = default;
	private:
		static const u32 ID;
	};

	template<typename T>
	const u32 Component<T>::ID{ ComponentBase::GetID() };

	struct TransformComponent : public Component<TransformComponent>
	{
		TransformComponent(DirectX::SimpleMath::Vector3 position = { 0.0f, 0.0f, 0.0f },
			DirectX::SimpleMath::Vector3 rotation = { 0.0f, 0.0f, 0.0f },
			DirectX::SimpleMath::Vector3 scale = { 1.0f, 1.0f, 1.0f }) noexcept
		{
			auto t = DirectX::SimpleMath::Matrix::CreateTranslation(position);
			auto r = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(rotation);
			auto s = DirectX::SimpleMath::Matrix::CreateScale(scale);
			worldMatrix = s * r * t;
		}
		TransformComponent& SetPosition(DirectX::SimpleMath::Vector3 position)
		{
			worldMatrix.Translation(position);
			return *this;
		}
		TransformComponent& SetRotation(DirectX::SimpleMath::Vector3 rotation)
		{
			DirectX::XMVECTOR scale, rotationQuat, translation;
			DirectX::XMMatrixDecompose(&scale, &rotationQuat, &translation, worldMatrix);
			worldMatrix = DirectX::XMMatrixScalingFromVector(scale) *
				DirectX::XMMatrixRotationRollPitchYawFromVector(rotation) *
				DirectX::XMMatrixTranslationFromVector(translation);
			return *this;
		}
		TransformComponent& SetScale(DirectX::SimpleMath::Vector3 scale)
		{
			DirectX::XMVECTOR xmScale, rotationQuat, translation;
			DirectX::XMMatrixDecompose(&xmScale, &rotationQuat, &translation, worldMatrix);
			worldMatrix = DirectX::XMMatrixScalingFromVector(scale) *
				DirectX::XMMatrixRotationQuaternion(rotationQuat) *
				DirectX::XMMatrixTranslationFromVector(translation);
			return *this;
		}

		DirectX::SimpleMath::Vector3 GetPosition()
		{
			return { worldMatrix(3, 0) , worldMatrix(3, 1), worldMatrix(3, 2) };
		}
		operator const DirectX::SimpleMath::Matrix& () const { return worldMatrix; }
		operator DirectX::SimpleMath::Matrix& () { return worldMatrix; }
		DirectX::SimpleMath::Matrix worldMatrix = DirectX::SimpleMath::Matrix::Identity;
	};

	struct ModelComponent : public Component<ModelComponent>
	{
		ModelComponent(u64 id = 0) noexcept : id{ id } {}
		operator const u64 () const { return id; }
		u64 id;
	};

	struct CameraComponent : public Component<CameraComponent>
	{
		using Matrix = DirectX::SimpleMath::Matrix;	

		Matrix viewMatrix = DirectX::XMMatrixIdentity();
		Matrix projMatrix = DirectX::XMMatrixIdentity();

		inline static CameraComponent* s_mainCamera = nullptr;
	};

	struct NetworkComponent : public Component<NetworkComponent>
	{
		int playerId;
	};

}