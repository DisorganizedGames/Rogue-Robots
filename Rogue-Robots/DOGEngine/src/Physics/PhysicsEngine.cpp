#include "PhysicsEngine.h"
#pragma warning(push, 0)
#include "BulletPhysics/btBulletDynamicsCommon.h"
#pragma warning(pop)
#include "../ECS/EntityManager.h"

namespace DOG
{
	PhysicsEngine PhysicsEngine::s_physicsEngine;

	PhysicsEngine::PhysicsEngine()
	{
		m_rigidBodyColliderDatas.resize(PhysicsEngine::RESIZE_RIGIDBODY_SIZE);
	}

	btDiscreteDynamicsWorld* PhysicsEngine::GetDynamicsWorld()
	{
		return s_physicsEngine.m_dynamicsWorld.get();
	}

	PhysicsEngine::~PhysicsEngine()
	{
		for (u32 i = 0; i < m_rigidBodyColliderDatas.size(); ++i)
		{
			btRigidBody* body = m_rigidBodyColliderDatas[i].rigidBody;
			if (body == nullptr)
				continue;

			if (body->getMotionState())
			{
				delete m_rigidBodyColliderDatas[i].motionState;
			}
			m_dynamicsWorld->removeRigidBody(body);
			delete m_rigidBodyColliderDatas[i].rigidBody;
			delete m_rigidBodyColliderDatas[i].collisionShape;
		}
		m_rigidBodyColliderDatas.clear();

		//m_dynamicsWorld.release();
		//m_sequentialImpulseContraintSolver.release();
		//m_broadphaseInterface.release();
		//m_collisionDispatcher.release();
		//m_collisionConfiguration.release();
	}

	void PhysicsEngine::Initialize()
	{
		s_physicsEngine.m_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
		s_physicsEngine.m_collisionDispatcher = std::make_unique<btCollisionDispatcher>(s_physicsEngine.m_collisionConfiguration.get());
		s_physicsEngine.m_broadphaseInterface = std::make_unique<btDbvtBroadphase>();
		s_physicsEngine.m_sequentialImpulseContraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
		s_physicsEngine.m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(s_physicsEngine.m_collisionDispatcher.get(),
			s_physicsEngine.m_broadphaseInterface.get(), s_physicsEngine.m_sequentialImpulseContraintSolver.get(), s_physicsEngine.m_collisionConfiguration.get());

		s_physicsEngine.m_dynamicsWorld->setGravity({0.0f, -9.82f, 0.0f});
	}

	void PhysicsEngine::UpdatePhysics(float deltaTime)
	{
		s_physicsEngine.GetDynamicsWorld()->stepSimulation(deltaTime, 10);

		EntityManager::Get().Collect<TransformComponent, BoxColliderComponent>().Do([&](TransformComponent& transform, BoxColliderComponent& collider)
			{
				//Get handle for vector
				u32 handle = s_physicsEngine.m_handleAllocator.GetSlot(collider.handle.handle);

				//Get rigidbody
				auto& rigidBody = s_physicsEngine.m_rigidBodyColliderDatas[handle];


				if (rigidBody.rigidBody && rigidBody.rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody.rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
				}
			});

		EntityManager::Get().Collect<TransformComponent, SphereColliderComponent>().Do([&](TransformComponent& transform, SphereColliderComponent& collider)
			{
				//Get handle for vector
				u32 handle = s_physicsEngine.m_handleAllocator.GetSlot(collider.handle.handle);

				//Get rigidbody
				auto& rigidBody = s_physicsEngine.m_rigidBodyColliderDatas[handle];


				if (rigidBody.rigidBody && rigidBody.rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody.rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
				}
			});

		EntityManager::Get().Collect<TransformComponent, CapsuleColliderComponent>().Do([&](TransformComponent& transform, CapsuleColliderComponent& collider)
			{
				//Get handle for vector
				u32 handle = s_physicsEngine.m_handleAllocator.GetSlot(collider.handle.handle);

				//Get rigidbody
				auto& rigidBody = s_physicsEngine.m_rigidBodyColliderDatas[handle];


				if (rigidBody.rigidBody && rigidBody.rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody.rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
				}
			});
	}

	RigidbodyHandle PhysicsEngine::AddRigidbodyColliderData(RigidbodyColliderData rigidbodyColliderData)
	{
		RigidbodyHandle rigidbodyHandle = s_physicsEngine.m_handleAllocator.Allocate<RigidbodyHandle>();
		u32 handle = gfx::HandleAllocator::GetSlot(rigidbodyHandle.handle);

		//Resize if needed
		if (handle >= s_physicsEngine.m_rigidBodyColliderDatas.size())
			s_physicsEngine.m_rigidBodyColliderDatas.resize(s_physicsEngine.m_rigidBodyColliderDatas.size() + PhysicsEngine::RESIZE_RIGIDBODY_SIZE);

		s_physicsEngine.m_rigidBodyColliderDatas[handle] = std::move(rigidbodyColliderData);

		return rigidbodyHandle;
	}

	RigidbodyHandle PhysicsEngine::AddRigidbody(entity entity, RigidbodyColliderData& rigidbodyColliderData, bool dynamic, float mass)
	{
		TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(entity);

		//Copy entity transform
		btTransform groundTransform;
		groundTransform.setFromOpenGLMatrix((float*)(&transform.worldMatrix));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = dynamic;
		float bodyMass = 0.0f;

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
		{
			rigidbodyColliderData.collisionShape->calculateLocalInertia(mass, localInertia);
			bodyMass = mass;
		}

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		rigidbodyColliderData.motionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(bodyMass, rigidbodyColliderData.motionState, rigidbodyColliderData.collisionShape, localInertia);
		rigidbodyColliderData.rigidBody = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		PhysicsEngine::GetDynamicsWorld()->addRigidBody(rigidbodyColliderData.rigidBody);

		return PhysicsEngine::AddRigidbodyColliderData(std::move(rigidbodyColliderData));
	}

	RigidbodyColliderData* PhysicsEngine::GetRigidbodyColliderData(u32 handle)
	{
		return &s_physicsEngine.m_rigidBodyColliderDatas[handle];
	}

	BoxColliderComponent::BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD; 
		rCD.collisionShape = new btBoxShape(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z));

		handle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	SphereColliderComponent::SphereColliderComponent(entity entity, float radius, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD;
		rCD.collisionShape = new btSphereShape(radius);

		handle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	CapsuleColliderComponent::CapsuleColliderComponent(entity entity, float radius, float height, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD;
		rCD.collisionShape = new btCapsuleShape(radius, height);

		handle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	RigidbodyComponent::RigidbodyComponent(entity enitity)
	{
		if (EntityManager::Get().HasComponent<BoxColliderComponent>(enitity))
		{
			handle = EntityManager::Get().GetComponent<BoxColliderComponent>(enitity).handle;
		}
		else if (EntityManager::Get().HasComponent<SphereColliderComponent>(enitity))
		{
			handle = EntityManager::Get().GetComponent<SphereColliderComponent>(enitity).handle;
		}
		else if (EntityManager::Get().HasComponent<CapsuleColliderComponent>(enitity))
		{
			handle = EntityManager::Get().GetComponent<CapsuleColliderComponent>(enitity).handle;
		}
		else
		{
			std::cout << "Entity has no collider component for rigidbody\n";
			assert(false);
		}
	}

	void RigidbodyComponent::ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation)
	{
		//Get handle for vector
		u32 rigidbodyHandle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(handle.handle);

		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(rigidbodyHandle);

		//Set no rotations in x,y,z
		float x = constrainXRotation ? 0.0f : 1.0f;
		float y = constrainYRotation ? 0.0f : 1.0f;
		float z = constrainZRotation ? 0.0f : 1.0f;

		rigidbodyColliderData->rigidBody->setAngularFactor(btVector3(x, y, z));
	}

	void RigidbodyComponent::ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition)
	{
		//Get handle for vector
		u32 rigidbodyHandle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(handle.handle);

		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(rigidbodyHandle);

		////Set freeze position in x,y,z
		float x = constrainXPosition ? 0.0f : 1.0f;
		float y = constrainYPosition ? 0.0f : 1.0f;
		float z = constrainZPosition ? 0.0f : 1.0f;

		rigidbodyColliderData->rigidBody->setLinearFactor(btVector3(x, y, z));
	}
}