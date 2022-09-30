#pragma once

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
	struct BoxColliderComponent
	{
		typedef u32 entity;
		BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass = 1.0f) noexcept;

		u32 handle;
		//btRigidBody* rigidBody;
		//std::unique_ptr<btRigidBody> rigidBody;
	};

	struct RigidbodyColliderData
	{
		std::unique_ptr<btRigidBody> rigidBody;
		std::unique_ptr<btDefaultMotionState> motionState;
		std::unique_ptr<btCollisionShape> collisionShape;
	};

	class BulletPhysics
	{
		friend BoxColliderComponent;
	private:
		std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
		std::unique_ptr<btCollisionDispatcher> m_collisionDispatcher;
		std::unique_ptr<btBroadphaseInterface> m_broadphaseInterface;
		std::unique_ptr<btSequentialImpulseConstraintSolver> m_sequentialImpulseContraintSolver;
		std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
		static BulletPhysics s_bulletPhysics;

		std::vector<RigidbodyColliderData> m_rigidBodyColliderDatas;

	private:
		BulletPhysics();
		static btDiscreteDynamicsWorld* GetDynamicsWorld();
		static u32 AddRigidbodyColliderData(RigidbodyColliderData rigidbodyColliderData);

	public:
		~BulletPhysics();
		BulletPhysics(const BulletPhysics& other) = delete;
		BulletPhysics& operator=(const BulletPhysics& other) = delete;
		static void Initialize();
		static void BulletTest();
		static void UpdatePhysics(float deltaTime);
	};
}