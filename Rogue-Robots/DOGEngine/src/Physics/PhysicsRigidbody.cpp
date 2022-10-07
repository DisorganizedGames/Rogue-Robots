#include "PhysicsRigidbody.h"
#pragma warning(push, 0)
#include "BulletPhysics/btBulletDynamicsCommon.h"
#pragma warning(pop)
#include "../ECS/EntityManager.h"

namespace DOG
{
	RigidbodyComponent::RigidbodyComponent(entity entity)
	{
		//Can only create a rigidbody component for box, sphere, capsule
		if (EntityManager::Get().HasComponent<BoxColliderComponent>(entity))
		{
			rigidbodyHandle = EntityManager::Get().GetComponent<BoxColliderComponent>(entity).rigidbodyHandle;
		}
		else if (EntityManager::Get().HasComponent<SphereColliderComponent>(entity))
		{
			rigidbodyHandle = EntityManager::Get().GetComponent<SphereColliderComponent>(entity).rigidbodyHandle;
		}
		else if (EntityManager::Get().HasComponent<CapsuleColliderComponent>(entity))
		{
			rigidbodyHandle = EntityManager::Get().GetComponent<CapsuleColliderComponent>(entity).rigidbodyHandle;
		}
		else
		{
			std::cout << "Entity has no collider component for rigidbody\n";
			assert(false);
		}

		//Get handle for vector
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(rigidbodyHandle.handle);

		//Set up rigidbody for collision
		PhysicsEngine::s_physicsEngine.m_rigidbodyCollision.insert({ handle, {} });
	}

	void RigidbodyComponent::SetOnCollisionEnter(std::function<void(entity, entity)> inOnCollisionEnter)
	{
		onCollisionEnter = inOnCollisionEnter;
	}

	void RigidbodyComponent::SetOnCollisionExit(std::function<void(entity, entity)> inOnCollisionExit)
	{
		onCollisionExit = inOnCollisionExit;
	}

	void RigidbodyComponent::ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation)
	{
		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(rigidbodyHandle);

		//Set no rotations in x,y,z
		float x = constrainXRotation ? 0.0f : 1.0f;
		float y = constrainYRotation ? 0.0f : 1.0f;
		float z = constrainZRotation ? 0.0f : 1.0f;

		rigidbodyColliderData->rigidBody->setAngularFactor(btVector3(x, y, z));
	}

	void RigidbodyComponent::ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition)
	{
		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(rigidbodyHandle);

		////Set freeze position in x,y,z
		float x = constrainXPosition ? 0.0f : 1.0f;
		float y = constrainYPosition ? 0.0f : 1.0f;
		float z = constrainZPosition ? 0.0f : 1.0f;

		rigidbodyColliderData->rigidBody->setLinearFactor(btVector3(x, y, z));
	}

	void PhysicsRigidbody::UpdateRigidbodies()
	{
		EntityManager::Get().Collect<RigidbodyComponent>().Do([&](RigidbodyComponent& rigidbody)
			{

			});
	}
}