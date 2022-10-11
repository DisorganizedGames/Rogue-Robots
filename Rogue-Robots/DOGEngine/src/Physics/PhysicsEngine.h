#pragma once
#include "../Graphics/Handles/HandleAllocator.h"

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
struct btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btRigidBody;
class btDefaultMotionState;
class btCollisionShape;

namespace DOG
{
	struct RigidbodyComponent;
	class PhysicsRigidbody;

	typedef u32 entity;

	struct RigidbodyHandle
	{
		u64 handle = 0;
		friend class TypedHandlePool;
	};

	struct CollisionShapeHandle
	{
		u64 handle = 0;
		friend class TypedHandlePool;
	};

	struct RigidbodyColliderData
	{
		btRigidBody* rigidBody = nullptr;
		btDefaultMotionState* motionState = nullptr;
		CollisionShapeHandle collisionShapeHandle;
		bool dynamic = false;
		entity rigidbodyEntity;
		DirectX::SimpleMath::Vector3 rigidbodyScale;
	};

	struct RigidbodyCollisionData
	{
		bool collisionCheck = false;
		bool activeCollision = false;
		RigidbodyHandle rigidbodyHandle;
	};

	struct BoxColliderComponent
	{
		BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic = false, float mass = 1.0f) noexcept;

		RigidbodyHandle rigidbodyHandle;
	};
	struct SphereColliderComponent
	{
		SphereColliderComponent(entity entity, float radius, bool dynamic = false, float mass = 1.0f) noexcept;

		RigidbodyHandle rigidbodyHandle;
	};
	struct CapsuleColliderComponent
	{
		CapsuleColliderComponent(entity entity, float radius, float height, bool dynamic = false, float mass = 1.0f) noexcept;

		RigidbodyHandle rigidbodyHandle;
	};
	struct MeshColliderComponent
	{
		MeshColliderComponent(entity entity, u32 modelID) noexcept;
		
		void LoadMesh(entity entity, u32 modelID);

		bool meshNotLoaded = true;
		RigidbodyHandle rigidbodyHandle;
	};

	struct MeshWaitData
	{
		entity meshEntity;
		u32 meshModelID;
	};

	struct MeshColliderData
	{
		u32 meshModelID = 0;
		CollisionShapeHandle collisionShapeHandle;
	};

	class PhysicsEngine
	{
		friend BoxColliderComponent;
		friend SphereColliderComponent;
		friend CapsuleColliderComponent;
		friend RigidbodyComponent;
		friend MeshColliderComponent;
		friend PhysicsRigidbody;

	private:
		//Order of unique ptrs matter for the destruction of the unique ptrs
		std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
		std::unique_ptr<btCollisionDispatcher> m_collisionDispatcher;
		std::unique_ptr<btBroadphaseInterface> m_broadphaseInterface;
		std::unique_ptr<btSequentialImpulseConstraintSolver> m_sequentialImpulseContraintSolver;
		std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
		static PhysicsEngine s_physicsEngine;

		std::vector<RigidbodyColliderData> m_rigidBodyColliderDatas;
		gfx::HandleAllocator m_handleAllocator;

		//Mesh colliders which are waiting for the models to be loaded in
		std::vector<MeshWaitData> m_meshCollidersWaitingForModels;

		//If the mesh already is an collider
		std::vector<MeshColliderData> m_meshCollidersLoadedInMemory;

		//To be able to reuse collision shapes (mostly for mesh colliders)
		std::vector<btCollisionShape*> m_collisionShapes;

		//Collision keeper for different rigidbodies
		std::unordered_map<u32, std::unordered_map<u32, RigidbodyCollisionData>> m_rigidbodyCollision;

		static constexpr u64 RESIZE_RIGIDBODY_SIZE = 1000;
		static constexpr u64 RESIZE_COLLISIONSHAPE_SIZE = 1000;

	private:
		PhysicsEngine();
		static void AddMeshColliderWaitForModel(const MeshWaitData& meshColliderData);
		static btDiscreteDynamicsWorld* GetDynamicsWorld();
		static RigidbodyHandle AddRigidbodyColliderData(RigidbodyColliderData rigidbodyColliderData);
		static RigidbodyHandle AddRigidbody(entity entity, RigidbodyColliderData& rigidbodyColliderData, bool dynamic, float mass);
		static RigidbodyColliderData* GetRigidbodyColliderData(const RigidbodyHandle& rigidbodyHandle);
		void CheckMeshColliders();
		static void AddMeshColliderData(const MeshColliderData& meshColliderData);
		static MeshColliderData GetMeshColliderData(u32 modelID);
		static CollisionShapeHandle AddCollisionShape(btCollisionShape* addCollisionShape);
		btCollisionShape* GetCollisionShape(const CollisionShapeHandle& collisionShapeHandle);
		void FreeRigidbodyData(const RigidbodyHandle& rigidbodyHandle, bool freeCollisionShape);
		void FreeCollisionShape(const CollisionShapeHandle& collisionShapeHandle);
		void CheckRigidbodyCollisions();

	public:
		~PhysicsEngine();
		PhysicsEngine(const PhysicsEngine& other) = delete;
		PhysicsEngine& operator=(const PhysicsEngine& other) = delete;
		static void Initialize();
		static void UpdatePhysics(float deltaTime);
		static void FreePhysicsFromEntity(entity entity);
	};
}