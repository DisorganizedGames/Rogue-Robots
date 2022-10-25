#include "PhysicsRigidbody.h"
#pragma warning(push, 0)
#include "BulletPhysics/btBulletDynamicsCommon.h"
#pragma warning(pop)
#include "../ECS/EntityManager.h"

namespace DOG
{
	RigidbodyComponent::RigidbodyComponent(entity entity, bool kinematicBody)
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

		constrainPositionX = constrainPositionY = constrainPositionZ = constrainRotationX = constrainRotationY = constrainRotationZ = false;

		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::s_physicsEngine.GetRigidbodyColliderData(rigidbodyHandle);
		if (kinematicBody)
		{
			assert(rigidbodyColliderData->dynamic && "Must be dynamic and kinematic");
			rigidbodyColliderData->rigidBody->setCollisionFlags(rigidbodyColliderData->rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			rigidbodyColliderData->rigidBody->setActivationState(DISABLE_DEACTIVATION);
			disableDeactivation = true;
			
		}
		else if (!rigidbodyColliderData->dynamic)
		{
			rigidbodyColliderData->rigidBody->setCollisionFlags(rigidbodyColliderData->rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
		}

		mass = rigidbodyColliderData->rigidBody->getMass();
	}

	//Fix later
	//void RigidbodyComponent::SetOnCollisionEnter(std::function<void(entity, entity)> inOnCollisionEnter)
	//{
	//	onCollisionEnter = inOnCollisionEnter;
	//}

	//void RigidbodyComponent::SetOnCollisionExit(std::function<void(entity, entity)> inOnCollisionExit)
	//{
	//	onCollisionExit = inOnCollisionExit;
	//}

	void RigidbodyComponent::ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation)
	{
		constrainRotationX = constrainXRotation;
		constrainRotationY = constrainYRotation;
		constrainRotationZ = constrainZRotation;
	}

	void RigidbodyComponent::ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition)
	{
		constrainPositionX = constrainXPosition;
		constrainPositionY = constrainYPosition;
		constrainPositionZ = constrainZPosition;
	}

	void PhysicsRigidbody::UpdateRigidbodies()
	{
		EntityManager::Get().Collect<RigidbodyComponent>().Do([&](RigidbodyComponent& rigidbody)
			{
				btRigidBody* bulletRigidbody = PhysicsEngine::GetRigidbodyColliderData(rigidbody.rigidbodyHandle)->rigidBody;

				bulletRigidbody->setLinearVelocity(btVector3(rigidbody.linearVelocity.x, rigidbody.linearVelocity.y, rigidbody.linearVelocity.z));
				bulletRigidbody->setAngularVelocity(btVector3(rigidbody.angularVelocity.x, rigidbody.angularVelocity.y, rigidbody.angularVelocity.z));
				bulletRigidbody->applyCentralForce(btVector3(rigidbody.centralForce.x, rigidbody.centralForce.y, rigidbody.centralForce.z));
				bulletRigidbody->applyCentralImpulse(btVector3(rigidbody.centralImpulse.x, rigidbody.centralImpulse.y, rigidbody.centralImpulse.z));
				bulletRigidbody->applyTorque(btVector3(rigidbody.torque.x, rigidbody.torque.y, rigidbody.torque.z));

				////Set freeze position in x,y,z
				float x = rigidbody.constrainPositionX ? 0.0f : 1.0f;
				float y = rigidbody.constrainPositionY ? 0.0f : 1.0f;
				float z = rigidbody.constrainPositionZ ? 0.0f : 1.0f;

				bulletRigidbody->setLinearFactor(btVector3(x, y, z));

				//Set no rotations in x,y,z
				x = rigidbody.constrainRotationX ? 0.0f : 1.0f;
				y = rigidbody.constrainRotationY ? 0.0f : 1.0f;
				z = rigidbody.constrainRotationZ ? 0.0f : 1.0f;

				bulletRigidbody->setAngularFactor(btVector3(x, y, z));

				//Can only turn off deactivation not turn it on again
				if (rigidbody.disableDeactivation)
					bulletRigidbody->setActivationState(DISABLE_DEACTIVATION);

				if (rigidbody.continuousCollisionDetection)
				{
					bulletRigidbody->setCcdMotionThreshold(rigidbody.continuousCollisionDetectionMotionThreshold);
					bulletRigidbody->setCcdSweptSphereRadius(rigidbody.continuousCollisionDetectionSweptSphereRadius);
				}
			});
	}

	void PhysicsRigidbody::UpdateValuesForRigidbodies()
	{
		EntityManager::Get().Collect<RigidbodyComponent>().Do([&](RigidbodyComponent& rigidbody)
			{
				btRigidBody* bulletRigidbody = PhysicsEngine::GetRigidbodyColliderData(rigidbody.rigidbodyHandle)->rigidBody;

				//Get the new velocity
				btVector3 linearVelocity = bulletRigidbody->getLinearVelocity();
				rigidbody.linearVelocity.x = linearVelocity.getX();
				rigidbody.linearVelocity.y = linearVelocity.getY();
				rigidbody.linearVelocity.z = linearVelocity.getZ();

				btVector3 turnVelocity = bulletRigidbody->getTurnVelocity();
				rigidbody.angularVelocity.x = turnVelocity.getX();
				rigidbody.angularVelocity.y = turnVelocity.getY();
				rigidbody.angularVelocity.z = turnVelocity.getZ();

				rigidbody.centralForce = DirectX::SimpleMath::Vector3::Zero;

				rigidbody.centralImpulse = DirectX::SimpleMath::Vector3::Zero;
				rigidbody.torque = DirectX::SimpleMath::Vector3::Zero;
			});
	}
}