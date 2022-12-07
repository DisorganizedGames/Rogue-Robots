#pragma once
#include "../Core/Types/GraphicsTypes.h"
#include "EntityTypedef.h"

namespace DOG
{
	enum class BlendMode
	{
		normal,
		linear,
		bezier,
		interrupt,
	};
	enum class AnimationFlag : u32
	{
		None = 0,
		Looping = 1 << 1,
		Persist = 1 << 2,
		ResetPrio = 1 << 3,
		SimpleAdd = 1 << 4,
		Interrupt = 1 << 5,
		ForceRestart = 1 << 6,
	};
	DEFINE_ENUM_FLAG_OPERATORS(AnimationFlag)
	

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
		DirectX::SimpleMath::Vector3 GetUp() const noexcept { return DirectX::SimpleMath::Vector3(worldMatrix._21, worldMatrix._22, worldMatrix._23); }
		DirectX::SimpleMath::Vector3 GetRight() const noexcept { return -worldMatrix.Right(); }
		DirectX::SimpleMath::Vector3 GetScale() const noexcept;

		TransformComponent& RotateW(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& RotateW(const DirectX::SimpleMath::Matrix& rotation) noexcept;
		TransformComponent& RotateL(const DirectX::SimpleMath::Vector3& rotation) noexcept;
		TransformComponent& RotateL(const DirectX::SimpleMath::Matrix& rotation) noexcept;

		TransformComponent& RotateForwardTo(const DirectX::SimpleMath::Vector3& target) noexcept;

		operator const DirectX::SimpleMath::Matrix& () const { return worldMatrix; }
		operator DirectX::SimpleMath::Matrix& () { return worldMatrix; }
		DirectX::SimpleMath::Matrix worldMatrix = DirectX::SimpleMath::Matrix::Identity;
	};

	struct ModelComponent
	{
		ModelComponent(u32 id = 0) noexcept : id{ id } {}
		operator const u32() const { return id; }
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
		const char* playerName;
	};

	struct NetworkTransform
	{
		u32 objectId = 0;
		DirectX::SimpleMath::Vector3 position;
	};
	struct DontDraw
	{
		bool dontDraw = true;
	};
	struct MixamoHeadJointTF
	{
		DirectX::SimpleMath::Matrix transform = DirectX::SimpleMath::Matrix::Identity;
	};
	struct RigDataComponent
	{
		u32 offset;
		i8 rigID = 0;
		i8 animatorID = -1;
	};
	struct AnimationComponent
	{
		static constexpr u8 MAX_SETTERS = 10;
		static constexpr u8 MAX_TARGET_ANIMS = 3;
		static constexpr u8 FULL_BODY = 0;
		static constexpr u8 LOWER_BODY = 1;
		static constexpr u8 UPPER_BODY = 2;
		static constexpr u8 BASE_PRIORITY = 0;
		u32 offset;
		i8 rigID = 0;
		i8 animatorID = -1;
		i8 addedSetters = 0;
		struct Setter
		{
			AnimationFlag flag = AnimationFlag::None;
			u8 group = 0;
			u8 priority = BASE_PRIORITY;
			f32 transitionLength = 0.f;
			f32 playbackRate = 1.f;
			i8 animationIDs[MAX_TARGET_ANIMS] = { -1, -1, -1 };
			f32 targetWeights[MAX_TARGET_ANIMS] = { 1.f, 1.f, 1.f };
		};
		std::array<Setter, MAX_SETTERS> animSetters;
		void SimpleAdd(i8 animationId, AnimationFlag flags = AnimationFlag::None, u32 priority = BASE_PRIORITY);
	};
	static_assert(std::is_trivially_copyable_v<AnimationComponent>);

	struct AudioComponent
	{
		u32 assetID = u32(-1);
		//f32 beginLoop = 0.f; These are not yet implemented
		//f32 endLoop = 0.f;
		
		//Old volume stuff
		//f32 volume = 2.0f;

		//Range from 0.0f -> 1.0f
		f32 volume = 1.0f;

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
	
	//Modular blocks
	struct ModularBlockComponent
	{
	};	//
	struct EmptySpaceComponent
	{
		DirectX::SimpleMath::Vector3 pos;
	};	//
	struct ThisPlayer
	{
	};
	struct OnlinePlayer
	{
	};
	struct ThisPlayerWeapon
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

	// Set component on meshes that should be outlined
	struct OutlineComponent
	{
		DirectX::SimpleMath::Vector3 color;
		bool onlyOutline{ false };
	};

	struct PointLightComponent
	{
		LightHandle handle;

		// Light properties
		DirectX::SimpleMath::Vector3 color{ 1.f, 1.f, 1.f };
		float strength{ 1.f };
		float radius{ 5.f };

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

	struct PickupLerpAnimateComponent
	{
		float baseOrigin{ 5.0f };
		float currentOrigin{ 5.0f };

		DirectX::SimpleMath::Vector3 origin{ 0,0,0 };
		DirectX::SimpleMath::Vector3 target{ 1,1,1 };

		float baseTarget{ 8.0f };

		float speed = 1.0f;
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
			return *this;
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
		DirectX::SimpleMath::Vector3 normal[maxCount]{};
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

	struct BoundingBoxComponent
	{
		using Vector3 = DirectX::SimpleMath::Vector3;

		DirectX::BoundingBox aabb;
		BoundingBoxComponent(Vector3 center, Vector3 extents);
		Vector3 Center();
		void Center(Vector3 c);
		Vector3 Extents();
		void Extents(Vector3 ext);
	};

#pragma region Particle System

	struct ParticleEmitterComponent
	{
		f32 spawnRate{ 0.f }; // How many particles to spawn on average per second
		f32 particleSize{ 1.f }; // Size of the particle in world space
		f32 particleLifetime{ 0.f }; // How long particles should live

		u32 textureHandle{ 0 }; // Handle to a raw texture view on the GPU
		u32 textureSegmentsX{ 1 }; // Tiles in X-axis
		u32 textureSegmentsY{ 1 }; // Tiles in Y-axis

		DirectX::SimpleMath::Vector4 startColor = { 1.f, 1.f, 1.f, 1.f };
		DirectX::SimpleMath::Vector4 endColor = { 1.f, 1.f, 1.f, 1.f };

		u32 emitterIndex{ static_cast<u32>(-1) }; // DO NOT TOUCH (used internally)
	};

	// --- SPAWN TYPES ---

	// Tells particles to spawn in a Cone with its tip on the entity's transform
	struct ConeSpawnComponent
	{
		f32 angle = DirectX::XM_PIDIV4;
		f32 speed = 1.f;
	};

	// Tells particles to spawn in a cylinder centered on the entity's transform
	struct CylinderSpawnComponent
	{
		f32 radius = 1.f;
		f32 height = 1.f;
	};
	
	// Tells particles to spawn in an Axis-aligned Box centered on the entity's transform
	struct BoxSpawnComponent
	{
		f32 x = 1.f;
		f32 y = 1.f;
		f32 z = 1.f;
	};


	// --- BEHAVIOR TYPES ---
	struct GravityBehaviorComponent
	{
		f32 gravity = 9.82f;
	};

	struct NoGravityBehaviorComponent {};

	struct GravityPointBehaviorComponent
	{
		DirectX::SimpleMath::Vector3 point = { 0.f, 0.f, 0.f };
		f32 gravity = 9.82f;
	};

	struct GravityDirectionBehaviorComponent
	{
		DirectX::SimpleMath::Vector3 direction = { 0.f, -1.f, 0.f }; // Will be normalized
		f32 gravity = 9.82f;
	};

	struct ConstVelocityBehaviorComponent
	{
		DirectX::SimpleMath::Vector3 velocity = { 0.f, 1.f, 0.f };
	};


	void ParticleSystemFromFile(entity e, const std::filesystem::path& path);

#pragma endregion
}
