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
		TransformComponent(const DirectX::SimpleMath::Vector3& position = { 0.0f, 0.0f, 0.0f },
			const DirectX::SimpleMath::Vector3& rotation = { 0.0f, 0.0f, 0.0f },
			const DirectX::SimpleMath::Vector3& scale = { 1.0f, 1.0f, 1.0f }) noexcept;
		TransformComponent& SetPosition(const DirectX::SimpleMath::Vector3& position) noexcept;
		TransformComponent& SetRotation(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& SetScale(const DirectX::SimpleMath::Vector3& scale) noexcept;
		DirectX::SimpleMath::Vector3 GetPosition() const noexcept;
		DirectX::SimpleMath::Matrix GetRotation() const noexcept;

		TransformComponent& RotateW(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& RotateW(const DirectX::SimpleMath::Matrix& rotation) noexcept;
		TransformComponent& RotateL(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& RotateL(const DirectX::SimpleMath::Matrix& rotation) noexcept;

		operator const DirectX::SimpleMath::Matrix& () const { return worldMatrix; }
		operator DirectX::SimpleMath::Matrix& () { return worldMatrix; }
		DirectX::SimpleMath::Matrix worldMatrix = DirectX::SimpleMath::Matrix::Identity;
	};

	struct ModelComponent : public Component<ModelComponent>
	{
		ModelComponent(u32 id = 0) noexcept : id{ id } {}
		operator const u32 () const { return id; }
		u32 id;
	};

	struct CameraComponent : public Component<CameraComponent>
	{
		using Matrix = DirectX::SimpleMath::Matrix;	

		Matrix viewMatrix = DirectX::XMMatrixIdentity();
		Matrix projMatrix = DirectX::XMMatrixIdentity();

		inline static CameraComponent* s_mainCamera = nullptr;
	};

	struct NetworkPlayerComponent : public Component<NetworkPlayerComponent>
	{
		int playerId;
	};

	struct NetworkComponent : public Component<NetworkComponent>
	{
	};

	struct AnimationComponent : public Component<AnimationComponent>
	{
		i32 offset;
		i32 animationID;
		f32 time;
		f32 timeScale;
		f32 transition;
	};
}