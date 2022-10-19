#pragma once
#include "../Core/Types/GraphicsTypes.h"

namespace DOG
{
	constexpr const u32 MAX_ENTITIES = 10'000u;
	constexpr const u32 NULL_ENTITY = MAX_ENTITIES;
	typedef u32 entity;
	enum class AnimationBlendMode
	enum class WeightBlendMode
	{
		normal,
		linear,
		bezier,
	};

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
		i32 offset = 0;
		f32 globalTime = 0.0f;
		struct AnimationClip
		{
			// Animation Specifics
			i32 animationID = 0;
			f32 totalTicks = 1.0f;
			f32 duration = 1.0f;
			// Clip Specifics
			f32 currentTick = 0.f;
			f32 normalizedTime = 0.f;
			f32 timeScale = 1.0f;
			f32 currentWeight = 1.0f;
			f32 targetWeight = 0.0f;
			bool loop = false;
			// Blend Specifics
			WeightBlendMode blendMode = WeightBlendMode::normal;
			f32 transitionStart = 0.0f;
			f32 transitionTime = 0.0f;
			bool matchingTime = false; //bs

			void UpdateClip(const f32 dt);
			void UpdateLinear(const f32 dt);
			void UpdateBezier(const f32 dt);
			bool HasActiveAnimation() const { return animationID != -1; };
			void SetAnimation(const i32 id, const f32 nTicks, const f32 duration, const f32 startTime = 0.0f);
		};
		std::array<AnimationClip, 3> clips;
		// Update 
		void Update(const f32 dt);
	};

	struct RealAnimationComponent
	{
		static constexpr u8 maxClips = 3;
		i32 offset = 0;
		f32 globalTime = 0.0f;
		struct AnimationClip
		{
			i32 group = 0;
			// Animation Specifics
			i32 animationID = 0;
			f32 totalTicks = 1.0f;
			f32 duration = 1.0f;
			// Clip Specifics
			f32 normalizedTime = 0.f;
			f32 currentTick = 0.0f;
			f32 timeScale = 1.0f;
			f32 startWeight = 1.0f;
			f32 targetWeight = 0.0f;
			f32 currentWeight = 1.0f;
			bool loop = false;
			bool activeAnimation = false;
			// Blend/transition Specifics
			WeightBlendMode blendMode = WeightBlendMode::normal;
			f32 transitionStart = 0.0f;
			f32 transitionLength = 0.0f;
			bool matchingTime = false; //bs

			f32 UpdateClipTick(const f32 gt);
			f32 UpdateWeightLinear(const f32 gt);
			f32 UpdateWeightBezier(const f32 gt);
			void UpdateState(const f32 gt, const f32 dt);
			void ResetClip();
			bool HasActiveAnimation() const { return animationID != -1; };
			void SetAnimation(const i32 id, const f32 nTicks, const f32 duration, const f32 startTime = 0.0f);
			bool operator <(const AnimationClip& o) const{
				return !o.activeAnimation ||
					o.activeAnimation && group < o.group ||
					o.activeAnimation && group == o.group && targetWeight > o.targetWeight ||
					o.activeAnimation && group == o.group && targetWeight == o.targetWeight && currentWeight > o.currentWeight;}
			bool operator ==(const AnimationClip& o) const{
				return animationID == o.animationID;}
			bool BecameActive(const f32 gt, const f32 dt) {
				return (gt > transitionStart) && (gt - dt < transitionStart);}
			bool Deactivated(const f32 gt, const f32 dt) {
				return activeAnimation && (gt+dt > transitionStart+transitionLength && !loop);}
		};
		std::array<AnimationClip, maxClips> clips;
		u32 animsPerGroup[maxClips] = { 0 };
		u32 nAddedClips = 0;
		// Update
		void Update(const f32 dt);
		void AddAnimation(i32 id, u32 group, f32 startDelay, f32 transitionLength, f32 startWeight, f32 targetWeight, bool loop = false, f32 timeScale = 1.f);
		i32 ActiveClipCount(){
			return animsPerGroup[0] + animsPerGroup[1] + animsPerGroup[2];
		}
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

