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
	typedef u32 entity;

	struct RigidbodyHandle
	{
		u64 handle = 0;
		friend class TypedHandlePool;
	};

	struct RigidbodyColliderData
	{
		btRigidBody* rigidBody = nullptr;
		btDefaultMotionState* motionState = nullptr;
		btCollisionShape* collisionShape = nullptr;
	};

	struct RigidbodyComponent
	{
		RigidbodyComponent(entity enitity);

		void ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation);
		void ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition);

		RigidbodyHandle handle;
	};

	struct BoxColliderComponent
	{
		BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass = 1.0f) noexcept;

		RigidbodyHandle handle;
	};
	struct SphereColliderComponent
	{
		SphereColliderComponent(entity entity, float radius, bool dynamic, float mass = 1.0f) noexcept;

		RigidbodyHandle handle;
	};
	struct CapsuleColliderComponent
	{
		CapsuleColliderComponent(entity entity, float radius, float height, bool dynamic, float mass = 1.0f) noexcept;

		RigidbodyHandle handle;
	};

	class PhysicsEngine
	{
		friend BoxColliderComponent;
		friend SphereColliderComponent;
		friend CapsuleColliderComponent;
		friend RigidbodyComponent;

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


	private:
		PhysicsEngine();
		static btDiscreteDynamicsWorld* GetDynamicsWorld();
		static RigidbodyHandle AddRigidbodyColliderData(RigidbodyColliderData rigidbodyColliderData);
		static RigidbodyHandle AddRigidbody(entity entity, RigidbodyColliderData& rigidbodyColliderData, bool dynamic, float mass);
		static RigidbodyColliderData* GetRigidbodyColliderData(u32 handle);

		static constexpr u64 RESIZE_RIGIDBODY_SIZE = 1000;

	public:
		~PhysicsEngine();
		PhysicsEngine(const PhysicsEngine& other) = delete;
		PhysicsEngine& operator=(const PhysicsEngine& other) = delete;
		static void Initialize();
		static void UpdatePhysics(float deltaTime);
	};
}