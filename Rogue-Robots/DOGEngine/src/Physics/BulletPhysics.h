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
	typedef u32 entity;

	struct RigidbodyColliderData
	{
		std::unique_ptr<btRigidBody> rigidBody;
		std::unique_ptr<btDefaultMotionState> motionState;
		std::unique_ptr<btCollisionShape> collisionShape;
	};

	struct RigidbodyComponent
	{
		RigidbodyComponent(entity enitity);

		void ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation);
		void ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition);

		u32 handle;
	};

	struct BoxColliderComponent
	{
		BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass = 1.0f) noexcept;

		u32 handle;
	};
	struct SphereColliderComponent
	{
		SphereColliderComponent(entity entity, float radius, bool dynamic, float mass = 1.0f) noexcept;

		u32 handle;
	};
	struct CapsuleColliderComponent
	{
		CapsuleColliderComponent(entity entity, float radius, float height, bool dynamic, float mass = 1.0f) noexcept;

		u32 handle;
	};

	class BulletPhysics
	{
		friend BoxColliderComponent;
		friend SphereColliderComponent;
		friend CapsuleColliderComponent;
		friend RigidbodyComponent;

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
		static u32 AddRigidbody(entity entity, RigidbodyColliderData& rigidbodyColliderData, bool dynamic, float mass);
		static RigidbodyColliderData* GetRigidbodyColliderData(u32 handle);

	public:
		~BulletPhysics();
		BulletPhysics(const BulletPhysics& other) = delete;
		BulletPhysics& operator=(const BulletPhysics& other) = delete;
		static void Initialize();
		static void BulletTest();
		static void UpdatePhysics(float deltaTime);
	};
}