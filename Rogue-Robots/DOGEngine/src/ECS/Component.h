#pragma once
#include "../Core/Types/GraphicsTypes.h"

namespace DOG
{
	constexpr const u32 MAX_ENTITIES = 10'000u;
	constexpr const u32 NULL_ENTITY = MAX_ENTITIES;
	typedef u32 entity;

	struct TransformComponent
	{
		TransformComponent(const DirectX::SimpleMath::Vector3& position = { 0.0f, 0.0f, 0.0f },
			const DirectX::SimpleMath::Vector3& rotation = { 0.0f, 0.0f, 0.0f },
			const DirectX::SimpleMath::Vector3& scale = { 1.0f, 1.0f, 1.0f }) noexcept;
		TransformComponent& SetPosition(const DirectX::SimpleMath::Vector3& position) noexcept;
		TransformComponent& SetRotation(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& SetRotation(const DirectX::SimpleMath::Matrix& rotationMatrix) noexcept;
		TransformComponent& SetScale(const DirectX::SimpleMath::Vector3& scale) noexcept;
		DirectX::SimpleMath::Vector3 GetPosition() const noexcept;
		DirectX::SimpleMath::Matrix GetRotation() const noexcept;
		DirectX::SimpleMath::Vector3 GetForward() const noexcept { return DirectX::SimpleMath::Vector3(worldMatrix._31, worldMatrix._32, worldMatrix._33); }
		DirectX::SimpleMath::Vector3 GetScale() const noexcept;

		TransformComponent& RotateW(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& RotateW(const DirectX::SimpleMath::Matrix& rotation) noexcept;
		TransformComponent& RotateL(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& RotateL(const DirectX::SimpleMath::Matrix& rotation) noexcept;

		operator const DirectX::SimpleMath::Matrix& () const { return worldMatrix; }
		operator DirectX::SimpleMath::Matrix& () { return worldMatrix; }
		DirectX::SimpleMath::Matrix worldMatrix = DirectX::SimpleMath::Matrix::Identity;
	};

	struct ModelComponent
	{
		ModelComponent(u32 id = 0) noexcept : id{ id } {}
		operator const u32 () const { return id; }
		u32 id;
	};

	struct CameraComponent
	{
		using Matrix = DirectX::SimpleMath::Matrix;	

		Matrix viewMatrix = DirectX::XMMatrixIdentity();
		Matrix projMatrix = DirectX::XMMatrixIdentity();

		bool isMainCamera = false;
	};

	struct NetworkPlayerComponent
	{
		i8 playerId;
	};

	struct NetworkTransform
	{
		u32 objectId  = 0;
		DirectX::SimpleMath::Matrix transform;
	};

	struct AnimationComponent
	{
		// initial animation component, liable to changge
		i32 offset = 0;
		i32 animationID[2] = { 0, -1 };
		f32 tick[2] = { 0.f, 0.f };
		f32 normalizedTime[2] = { 0.f, 0.f };
		f32 timeScale[2] = { 1.0f, 1.0f };
		f32 transition = 0.0f;
		i32 mode = 0;
		f32 bf = 0.0f;
		/*bool HasActiveAnimation(const u8 animation) const noexcept{
			return animationID[animation] != -1; };
		void UpdateBlend(const f32 dt, const f32 animDuration, const f32 animTicks)
		{

		}*/
	};

	struct AudioComponent
	{
		u32 assetID = u32(-1);
		//f32 beginLoop = 0.f; These are not yet implemented
		//f32 endLoop = 0.f;
		f32 volume = 2.0f;

		u32 source = u32(-1);

		f32 loopStart = 0.0;
		f32 loopEnd = 0.0;

		bool shouldPlay = false;
		bool playing = false;
		bool shouldStop = false;

		bool loop = false;
		
		bool is3D = false; // Uses the object's transform as a sound source
	};

	struct AudioListenerComponent
	{
		// Maybe add options here like cone and such
	};

	struct ModularBlockComponent
	{
	};	//
	struct ThisPlayer
	{
	};
	struct OnlinePlayer
	{
	};

	struct SpotLightComponent
	{
		LightHandle handle;

		// Light properties
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 1.f };
		DirectX::SimpleMath::Vector3 direction{ 0.f, 0.f, 1.f };
		float cutoffAngle{ 15.f };
		u32 id = (u32)- 1;

		bool dirty{ true };		// If static handle, dirty bool is ignored
		bool isMainPlayerSpotlight{ false };
		entity owningPlayer { NULL_ENTITY };
	};

	struct PointLightComponent
	{
		LightHandle handle;

		// Light properties
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 1.f };

		bool dirty{ true };		// If static handle, dirty bool is ignored
	};

	struct LerpAnimateComponent
	{
		DirectX::SimpleMath::Vector3 origin{ 0,0,0 };
		DirectX::SimpleMath::Vector3 target{ 1,1,1 };
		f64 t{ 0 };
		f64 scale { 1 };
		i32 loops{ 1 };
	};

	struct LerpColorComponent
	{
		DirectX::SimpleMath::Vector3 origin{ 0,0,0 };
		DirectX::SimpleMath::Vector3 target{ 1,1,1 };
		f64 t{ 0 };
		f64 scale{ 1 };
		i32 loops{ 1 };
	};

	struct DirtyComponent
	{
		static constexpr u8 positionChanged = 0;
		static constexpr u8 rotationChanged = 1;
		DirtyComponent& SetDirty(u8 index)
		{
			dirtyBitSet[index] = true;
		}
		bool IsDirty(u8 index)
		{
			return dirtyBitSet[index];
		}
		std::bitset<2> dirtyBitSet;
	};

	//Is set on entities which are going to be destroyed at the end of the frame!
	struct DeferredDeletionComponent
	{

	};

	
	// Single submesh single material helper
	struct SubmeshRenderer
	{
		Mesh mesh;

		MaterialHandle material;
		MaterialDesc materialDesc;

		bool dirty{ false };
	};

	struct HasEnteredCollisionComponent
	{
		static constexpr u32 maxCount = 10;
		u32 entitiesCount{ 0 };
		entity entities[maxCount] = { NULL_ENTITY };
	};
	
	struct ShadowReceiverComponent
	{
		//A.t.m. only an identifier
	};

	struct ShadowCasterComponent
	{
		//A.t.m. only an identifier
	};

	struct MeshFilter
	{
		Mesh mesh;
		u32 numSubmeshes{ 0 };
	};

	struct MeshRenderer
	{
		// Modify MaterialDesc and call SetDirty() to dynamically change material arguments
		struct MRMaterial
		{
			bool dirty{ false };
			MaterialHandle handle;
			MaterialDesc material;

			void SetDirty() { dirty = true; };
		};

		// Size of materials must be >= accompanied MeshFilter num submeshes
		std::vector<MRMaterial> materials;
		
		// Shadows
		bool isShadowed{ false };
		bool doubleSidedShadows{ false };
	};

	struct SkeletonMeshRenderer
	{
		
	};
}

