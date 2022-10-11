#include "PhysicsEngine.h"
#pragma warning(push, 0)
#include "BulletPhysics/btBulletDynamicsCommon.h"
#pragma warning(pop)
#include "../ECS/EntityManager.h"
#include "../Core/AssetManager.h"
#include "../Scripting/LuaMain.h"
#include "PhysicsRigidbody.h"

namespace DOG
{
	PhysicsEngine PhysicsEngine::s_physicsEngine;

	PhysicsEngine::PhysicsEngine()
	{
		m_rigidBodyColliderDatas.resize(PhysicsEngine::RESIZE_RIGIDBODY_SIZE);

		m_collisionShapes.resize(PhysicsEngine::RESIZE_COLLISIONSHAPE_SIZE);

		m_ghostObjectDatas.resize(PhysicsEngine::RESIZE_GHOST_OBJECT_SIZE);
	}

	void PhysicsEngine::AddMeshColliderWaitForModel(const MeshWaitData& meshColliderData)
	{
		s_physicsEngine.m_meshCollidersWaitingForModels.push_back(meshColliderData);
	}

	btDiscreteDynamicsWorld* PhysicsEngine::GetDynamicsWorld()
	{
		return s_physicsEngine.m_dynamicsWorld.get();
	}

	PhysicsEngine::~PhysicsEngine()
	{
		//Delete rigidbodys
		for (u32 i = 0; i < m_rigidBodyColliderDatas.size(); ++i)
		{
			btRigidBody* body = m_rigidBodyColliderDatas[i].rigidBody;
			if (body == nullptr)
				continue;

			if (body->getMotionState())
			{
				delete m_rigidBodyColliderDatas[i].motionState;
				m_rigidBodyColliderDatas[i].motionState = nullptr;
			}
			m_dynamicsWorld->removeRigidBody(body);
			delete m_rigidBodyColliderDatas[i].rigidBody;
			m_rigidBodyColliderDatas[i].rigidBody = nullptr;
		}

		//Delete collisionShapes
		for (u32 i = 0; i < m_collisionShapes.size(); ++i)
		{
			if (m_collisionShapes[i])
			{
				delete m_collisionShapes[i];
				m_collisionShapes[i] = nullptr;
			}
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

		//For checking trigger collisions (i'm pretty sure)
		s_physicsEngine.m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
	}

	void PhysicsEngine::UpdatePhysics(float deltaTime)
	{
		s_physicsEngine.CheckMeshColliders();

		PhysicsRigidbody::UpdateRigidbodies();

		//Is possible that this is removed later 
		{
			EntityManager::Get().Collect<TransformComponent, BoxColliderComponent>().Do([&](TransformComponent& transform, BoxColliderComponent& collider)
				{
					//Get rigidbody
					auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(collider.rigidbodyHandle);
					if (rigidBody->dynamic && rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
					{
						btTransform trans;
						trans.setFromOpenGLMatrix((float*)(&transform.worldMatrix));
						rigidBody->rigidBody->getMotionState()->setWorldTransform(trans);
					}
				});

			EntityManager::Get().Collect<TransformComponent, SphereColliderComponent>().Do([&](TransformComponent& transform, SphereColliderComponent& collider)
				{
					//Get rigidbody
					auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(collider.rigidbodyHandle);
					if (rigidBody->dynamic && rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
					{
						btTransform trans;
						trans.setFromOpenGLMatrix((float*)(&transform.worldMatrix));
						rigidBody->rigidBody->getMotionState()->setWorldTransform(trans);
					}
				});

			EntityManager::Get().Collect<TransformComponent, CapsuleColliderComponent>().Do([&](TransformComponent& transform, CapsuleColliderComponent& collider)
				{
					//Get rigidbody
					auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(collider.rigidbodyHandle);
					if (rigidBody->dynamic && rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
					{
						btTransform trans;
						trans.setFromOpenGLMatrix((float*)(&transform.worldMatrix));
						rigidBody->rigidBody->getMotionState()->setWorldTransform(trans);
					}
				});
		}

		s_physicsEngine.GetDynamicsWorld()->stepSimulation(deltaTime, 10);

		EntityManager::Get().Collect<TransformComponent, BoxColliderComponent>().Do([&](TransformComponent& transform, BoxColliderComponent& collider)
			{
				//Get rigidbody
				auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(collider.rigidbodyHandle);
				if (rigidBody->dynamic && rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody->rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
					//The scale is set to 1 by bullet physics, so we set it back to the original scale
					transform.SetScale(rigidBody->rigidbodyScale);
				}
			});
		
		EntityManager::Get().Collect<TransformComponent, SphereColliderComponent>().Do([&](TransformComponent& transform, SphereColliderComponent& collider)
			{
				//Get rigidbody
				auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(collider.rigidbodyHandle);
				if (rigidBody->dynamic && rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody->rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
					//The scale is set to 1 by bullet physics, so we set it back to the original scale
					transform.SetScale(rigidBody->rigidbodyScale);
				}
			});

		EntityManager::Get().Collect<TransformComponent, CapsuleColliderComponent>().Do([&](TransformComponent& transform, CapsuleColliderComponent& collider)
			{
				//Get rigidbody
				auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(collider.rigidbodyHandle);
				if (rigidBody->dynamic && rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody->rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
					//The scale is set to 1 by bullet physics, so we set it back to the original scale
					transform.SetScale(rigidBody->rigidbodyScale);
				}
			});

		s_physicsEngine.CheckRigidbodyCollisions();
		s_physicsEngine.CheckGhostObjectCollisions();

		//Because for now we do not have any callbacks to c++ we only have to check if entity have a ScriptComponent
		EntityManager::Get().Collect<RigidbodyComponent, ScriptComponent>().Do([&](RigidbodyComponent& rigidbody, ScriptComponent&)
			{
				//Get rigidbody
				auto* rigidBody = s_physicsEngine.GetRigidbodyColliderData(rigidbody.rigidbodyHandle);
				if (rigidBody->rigidBody && rigidBody->rigidBody->getMotionState())
				{
					//Get handle for vector
					u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(rigidbody.rigidbodyHandle.handle);
					auto collisions = s_physicsEngine.m_rigidbodyCollision.find(handle);

					//Check if the existing collisions are exiting or have just entered the collision
					for (auto it = collisions->second.begin(); it != collisions->second.end();)
					{
						bool incrementIterator = true;

						//Check if the activeCollision has changed, if it has then we either call onCollisionEnter or onCollisionExit
						bool beforeCollision = it->second.activeCollision;
						it->second.activeCollision = it->second.collisionCheck;
						if (it->second.activeCollision != beforeCollision)
						{
							//Get RigidbodyColliderData to get the corresponding entities
							RigidbodyColliderData* obj0RigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(rigidbody.rigidbodyHandle);
							RigidbodyColliderData* obj1RigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(it->second.rigidbodyHandle);

							//Fix later
							//if (it->second.activeCollision && rigidbody.onCollisionEnter != nullptr)
							//	rigidbody.onCollisionEnter(obj0RigidbodyColliderData->rigidbodyEntity, obj1RigidbodyColliderData->rigidbodyEntity);
							if (it->second.activeCollision)
								LuaMain::GetScriptManager()->CallFunctionOnAllEntityScripts(obj0RigidbodyColliderData->rigidbodyEntity, "OnCollisionEnter", obj1RigidbodyColliderData->rigidbodyEntity);

							//Set collisionCheck false for next collision check
							it->second.collisionCheck = false;

							if (!it->second.activeCollision)
							{
								//Fix later
								//if (rigidbody.onCollisionExit != nullptr)
								//	rigidbody.onCollisionExit(obj0RigidbodyColliderData->rigidbodyEntity, obj1RigidbodyColliderData->rigidbodyEntity);
								//else
								LuaMain::GetScriptManager()->CallFunctionOnAllEntityScripts(obj0RigidbodyColliderData->rigidbodyEntity, "OnCollisionExit", obj1RigidbodyColliderData->rigidbodyEntity);

								//Remove the collision because we do not need to keep track of it anymore
 								collisions->second.erase(it++);
								incrementIterator = false;
							}
						}
						else
						{
							//Set collisionCheck false for next collision check
							it->second.collisionCheck = false;
						}

						if (incrementIterator)
							++it;
					}
				}
			});

		PhysicsRigidbody::UpdateValuesForRigidbodies();
	}

	void PhysicsEngine::FreePhysicsFromEntity(entity entity)
	{
		if (EntityManager::Get().HasComponent<BoxColliderComponent>(entity))
		{
			BoxColliderComponent& colliderComponent = EntityManager::Get().GetComponent<BoxColliderComponent>(entity);
			s_physicsEngine.FreeRigidbodyData(colliderComponent.rigidbodyHandle, true);
		}
		if (EntityManager::Get().HasComponent<SphereColliderComponent>(entity))
		{
			SphereColliderComponent& colliderComponent = EntityManager::Get().GetComponent<SphereColliderComponent>(entity);
			s_physicsEngine.FreeRigidbodyData(colliderComponent.rigidbodyHandle, true);
		}
		if (EntityManager::Get().HasComponent<CapsuleColliderComponent>(entity))
		{
			CapsuleColliderComponent& colliderComponent = EntityManager::Get().GetComponent<CapsuleColliderComponent>(entity);
			s_physicsEngine.FreeRigidbodyData(colliderComponent.rigidbodyHandle, true);
		}
		if (EntityManager::Get().HasComponent<MeshColliderComponent>(entity))
		{
			MeshColliderComponent& colliderComponent = EntityManager::Get().GetComponent<MeshColliderComponent>(entity);
			s_physicsEngine.FreeRigidbodyData(colliderComponent.rigidbodyHandle, false);
		}
		if (EntityManager::Get().HasComponent<RigidbodyComponent>(entity))
		{
			RigidbodyComponent& rigidbodyComponent = EntityManager::Get().GetComponent<RigidbodyComponent>(entity);
			//Get handle for vector
			u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(rigidbodyComponent.rigidbodyHandle.handle);
			//Clear collision checks if there should be any!
			s_physicsEngine.m_rigidbodyCollision.find(handle)->second.clear();
		}
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
		//Set rigidody entity
		rigidbodyColliderData.rigidbodyEntity = entity;

		TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(entity);

		//Copy entity transform
		btTransform groundTransform;
		
		// This is a hack until it's properly fixed
		TransformComponent test = transform;
		test.SetScale({ 1, 1, 1 });

		groundTransform.setFromOpenGLMatrix((float*)(&test.worldMatrix));

		rigidbodyColliderData.rigidbodyScale = transform.GetScale();

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = dynamic;
		float bodyMass = 0.0f;

		btCollisionShape* collisionShape = s_physicsEngine.GetCollisionShape(rigidbodyColliderData.collisionShapeHandle);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
		{
			collisionShape->calculateLocalInertia(mass, localInertia);
			bodyMass = mass;
		}

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		rigidbodyColliderData.motionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(bodyMass, rigidbodyColliderData.motionState, collisionShape, localInertia);
		rigidbodyColliderData.rigidBody = new btRigidBody(rbInfo);

		//Keep track if the rigidbody is dynamic or not
		rigidbodyColliderData.dynamic = dynamic;

		//add the body to the dynamics world
		PhysicsEngine::GetDynamicsWorld()->addRigidBody(rigidbodyColliderData.rigidBody);

		//Get the handle
		RigidbodyHandle rigidbodyHandle = PhysicsEngine::AddRigidbodyColliderData(std::move(rigidbodyColliderData));

		RigidbodyColliderData* pointerToRCD= PhysicsEngine::GetRigidbodyColliderData(rigidbodyHandle);

		//Sets the handle for collision detection later
		const u64 bitMask = 0x0000FFFF;
		const u32 byteShift = 4;
		u32 userIndex1 = (u32)(rigidbodyHandle.handle & bitMask);
		u32 userIndex2 = (u32)((rigidbodyHandle.handle >> byteShift) & bitMask);
		pointerToRCD->rigidBody->setUserIndex(userIndex1);
		pointerToRCD->rigidBody->setUserIndex2(userIndex2);
		pointerToRCD->rigidBody->setUserIndex3(true);

		return rigidbodyHandle;
	}

	GhostObjectHandle PhysicsEngine::AddGhostObjectData(GhostObjectData& ghostObjectData)
	{
		GhostObjectHandle ghostObjectHandle = s_physicsEngine.m_handleAllocator.Allocate<GhostObjectHandle>();
		u32 handle = gfx::HandleAllocator::GetSlot(ghostObjectHandle.handle);

		//Resize if needed
		if (handle >= s_physicsEngine.m_ghostObjectDatas.size())
			s_physicsEngine.m_ghostObjectDatas.resize(s_physicsEngine.m_ghostObjectDatas.size() + PhysicsEngine::RESIZE_GHOST_OBJECT_SIZE);

		s_physicsEngine.m_ghostObjectDatas[handle] = std::move(ghostObjectData);

		//Increase the counter for ghost objects
		++s_physicsEngine.m_nrGhostObjects;

		return ghostObjectHandle;
	}

	GhostObjectHandle PhysicsEngine::AddGhostObject(entity entity, GhostObjectData& ghostObjectData)
	{
		//Set rigidody entity
		ghostObjectData.ghostObjectEntity = entity;

		ghostObjectData.ghostObject = new btPairCachingGhostObject();

		//Set collision shape
		btCollisionShape* collider = s_physicsEngine.GetCollisionShape(ghostObjectData.collisionShapeHandle);
		ghostObjectData.ghostObject->setCollisionShape(collider);

		TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(entity);

		//Copy entity transform
		btTransform groundTransform;
		groundTransform.setFromOpenGLMatrix((float*)(&transform.worldMatrix));
		ghostObjectData.ghostObject->setWorldTransform(groundTransform);

		//Add it to the world
		s_physicsEngine.m_dynamicsWorld->addCollisionObject(ghostObjectData.ghostObject);
		ghostObjectData.ghostObject->setCollisionFlags(ghostObjectData.ghostObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

		GhostObjectHandle ghostObjectHandle = PhysicsEngine::AddGhostObjectData(ghostObjectData);

		//Sets the handle for collision detection later
		const u64 bitMask = 0x0000FFFF;
		const u32 byteShift = 4;
		u32 userIndex1 = (u32)(ghostObjectHandle.handle & bitMask);
		u32 userIndex2 = (u32)((ghostObjectHandle.handle >> byteShift) & bitMask);
		ghostObjectData.ghostObject->setUserIndex(userIndex1);
		ghostObjectData.ghostObject->setUserIndex2(userIndex2);
		ghostObjectData.ghostObject->setUserIndex3(false);

		return ghostObjectHandle;
	}

	RigidbodyColliderData* PhysicsEngine::GetRigidbodyColliderData(const RigidbodyHandle& rigidbodyHandle)
	{
		//Get handle for vector
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(rigidbodyHandle.handle);

		return &s_physicsEngine.m_rigidBodyColliderDatas[handle];
	}

	GhostObjectData* PhysicsEngine::GetGhostObjectData(const GhostObjectHandle& ghostObjectHandle)
	{
		//Get handle for vector
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(ghostObjectHandle.handle);

		return &s_physicsEngine.m_ghostObjectDatas[handle];
	}

	void PhysicsEngine::CheckMeshColliders()
	{
		//Check mesh colliders until the model is loaded into memory
		for (u32 index = 0; index < m_meshCollidersWaitingForModels.size(); ++index)
		{
			ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(m_meshCollidersWaitingForModels[index].meshModelID);
			if (model)
			{
				MeshColliderComponent component = EntityManager::Get().GetComponent<MeshColliderComponent>(m_meshCollidersWaitingForModels[index].meshEntity);
				component.LoadMesh(m_meshCollidersWaitingForModels[index].meshEntity, m_meshCollidersWaitingForModels[index].meshModelID);
				m_meshCollidersWaitingForModels.erase(m_meshCollidersWaitingForModels.begin() + index);
				--index;
			}
		}
	}

	void PhysicsEngine::AddMeshColliderData(const MeshColliderData& meshColliderData)
	{
		s_physicsEngine.m_meshCollidersLoadedInMemory.push_back(meshColliderData);
	}

	MeshColliderData PhysicsEngine::GetMeshColliderData(u32 modelID)
	{
		//Get mesh collider data if it exists
		for (auto& data : s_physicsEngine.m_meshCollidersLoadedInMemory)
		{
			if (data.meshModelID == modelID)
			{
				return data;
			}
		}

		MeshColliderData meshColliderData;
		meshColliderData.collisionShapeHandle.handle = 0;
		meshColliderData.meshModelID = 0;
		return meshColliderData;
	}

	CollisionShapeHandle PhysicsEngine::AddCollisionShape(btCollisionShape* addCollisionShape)
	{
		CollisionShapeHandle collisionShapeHandle = s_physicsEngine.m_handleAllocator.Allocate<CollisionShapeHandle>();
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(collisionShapeHandle.handle);

		//Resize if needed
		if (handle >= s_physicsEngine.m_collisionShapes.size())
			s_physicsEngine.m_collisionShapes.resize(s_physicsEngine.m_collisionShapes.size() + PhysicsEngine::RESIZE_COLLISIONSHAPE_SIZE);

		s_physicsEngine.m_collisionShapes[handle] = addCollisionShape;

		return collisionShapeHandle;
	}

	btCollisionShape* PhysicsEngine::GetCollisionShape(const CollisionShapeHandle& collisionShapeHandle)
	{
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(collisionShapeHandle.handle);

		//0 is default for HandleAllocator
		if (handle == 0)
		{
			std::cout << "Handle does not exist!\n";
			assert(false);
		}

		return m_collisionShapes[handle];
	}

	void PhysicsEngine::FreeRigidbodyData(const RigidbodyHandle& rigidbodyHandle, bool freeCollisionShape)
	{
		RigidbodyColliderData* rigidbodyColliderData = GetRigidbodyColliderData(rigidbodyHandle);
		if (rigidbodyColliderData->motionState)
		{
			delete rigidbodyColliderData->motionState;
			rigidbodyColliderData->motionState = nullptr;
		}
		m_dynamicsWorld->removeRigidBody(rigidbodyColliderData->rigidBody);
		delete rigidbodyColliderData->rigidBody;
		rigidbodyColliderData->rigidBody = nullptr;

		if (freeCollisionShape)
			FreeCollisionShape(rigidbodyColliderData->collisionShapeHandle);

		m_handleAllocator.Free(rigidbodyHandle);
	}

	void PhysicsEngine::FreeCollisionShape(const CollisionShapeHandle& collisionShapeHandle)
	{
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(collisionShapeHandle.handle);

		//0 is default for HandleAllocator
		if (handle == 0)
		{
			std::cout << "Handle does not exist!\n";
			assert(false);
		}

		delete (m_collisionShapes[handle]);
		m_collisionShapes[handle] = nullptr;

		m_handleAllocator.Free(collisionShapeHandle);
	}

	void PhysicsEngine::CheckRigidbodyCollisions()
	{
		int num_manifolds = s_physicsEngine.m_dynamicsWorld->getDispatcher()->getNumManifolds();
		for (int i = 0; i < num_manifolds; i++)
		{
			btPersistentManifold* contact_manifold = s_physicsEngine.m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
			btCollisionObject* obj0 = (btCollisionObject*)(contact_manifold->getBody0());
			btCollisionObject* obj1 = (btCollisionObject*)(contact_manifold->getBody1());

			for (int j = 0; j < contact_manifold->getNumContacts(); j++)
			{
				btManifoldPoint& pt = contact_manifold->getContactPoint(j);
				if (pt.getDistance() < 0.0f)
				{
					//Get obj0 rigidbody handle
					const u32 byteShift = 4;
					u64 obj0RigidbodyHandle = (obj0->getUserIndex2() << byteShift) | obj0->getUserIndex();

					//Get handle for obj0
					u32 obj0handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(obj0RigidbodyHandle);

					//Get the collisionKeeper for obj0
					auto collisions = PhysicsEngine::s_physicsEngine.m_rigidbodyCollision.find(obj0handle);

					if (collisions == PhysicsEngine::s_physicsEngine.m_rigidbodyCollision.end())
						continue;

					//Get obj1 rigidbody handle
					u64 obj1RigidbodyHandle = (obj1->getUserIndex2() << byteShift) | obj1->getUserIndex();
					//Get handle for obj1
					u32 obj1Handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(obj1RigidbodyHandle);

					//Check if obj1 exist in the collisionKeeper if it does then we set the collisionCheck true
					auto obj1Collision = collisions->second.find(obj1Handle);
					if (obj1Collision != collisions->second.end())
					{
						obj1Collision->second.collisionCheck = true;
					}
					else
					{
						//Add new collision to the collisionKeeper
						RigidbodyCollisionData collisionData;
						collisionData.activeCollision = false;
						collisionData.collisionCheck = true;
						RigidbodyHandle handle;
						handle.handle = obj1RigidbodyHandle;
						collisionData.rigidbodyHandle = handle;
						collisions->second.insert({ obj1Handle, collisionData });
					}

					//If the two objects did collide then we do not need to check the other collision points
					break;
				}
			}
		}
	}

	void PhysicsEngine::CheckGhostObjectCollisions()
	{
		static btManifoldArray manifoldsArray;
		u64 checkedGhostObjects = 0;
		//Handles start at one
		u64 ghostObjectIndex = 1;
		while (checkedGhostObjects < m_nrGhostObjects)
		{
			if (m_ghostObjectDatas[ghostObjectIndex].ghostObject != nullptr)
			{
				GhostObjectData* ghostObjectData = &m_ghostObjectDatas[ghostObjectIndex];
				m_dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(ghostObjectData->ghostObject->getOverlappingPairCache(), m_dynamicsWorld->getDispatchInfo(), m_dynamicsWorld->getDispatcher());

				for (int i = 0; i < ghostObjectData->ghostObject->getOverlappingPairCache()->getNumOverlappingPairs(); ++i)
				{
					btBroadphasePair* collisionPair = &ghostObjectData->ghostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];
					
					btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
					btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

					if (!obj0->getUserIndex3() && !obj1->getUserIndex3())
					{
						std::cout << "Two ghosts\n";
						continue;
					}

					const u32 byteShift = 4;
					//Get obj1 rigidbody handle
					u64 obj0CollisionObjectHandle = (obj0->getUserIndex2() << byteShift) | obj0->getUserIndex();
					//Get handle for obj1
					u32 obj0Handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(obj0CollisionObjectHandle);

					if (obj0->getUserIndex3())
					{
						RigidbodyColliderData* obj = PhysicsEngine::s_physicsEngine.GetRigidbodyColliderData((RigidbodyHandle)obj0CollisionObjectHandle);
						if (EntityManager::Get().HasComponent<RigidbodyComponent>(obj->rigidbodyEntity))
						{
							std::cout << "obj0 has an rigidbody\n";
						}
					}

					//Get obj1 rigidbody handle
					u64 obj1CollisionObjectHandle = (obj1->getUserIndex2() << byteShift) | obj1->getUserIndex();
					//Get handle for obj1
					u32 obj1Handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(obj1CollisionObjectHandle);

					if (obj1->getUserIndex3())
					{
						RigidbodyColliderData* obj = PhysicsEngine::s_physicsEngine.GetRigidbodyColliderData((RigidbodyHandle)obj1CollisionObjectHandle);
						if (EntityManager::Get().HasComponent<RigidbodyComponent>(obj->rigidbodyEntity))
						{
							std::cout << "obj1 has an rigidbody\n";
						}
					}

					//Check for flags here???

					if (collisionPair->m_algorithm)
					{
						collisionPair->m_algorithm->getAllContactManifolds(manifoldsArray);
					}
					else
					{
						continue;
					}

					for (int j = 0; j < manifoldsArray.size(); ++j)
					{
						btPersistentManifold* manifold = manifoldsArray[j];
						for (int k = 0; k < manifold->getNumContacts(); ++k)
						{
							const btManifoldPoint& pt = manifold->getContactPoint(k);

							if (pt.getDistance() < 0.0f)
							{
								std::cout << "Collisions Check\n";
								//If the two objects did collide then we do not need to check the other collision points
								j = manifoldsArray.size();
								break;
							}
						}
					}
				}

				++checkedGhostObjects;
			}

			++ghostObjectIndex;
		}

		//m_dynamicsWorld->setInternalTickCallback

		//btBroadphasePairArray& collisionPairs = m_ghostObject->getOverlappingPairCache()->getOverlappingPairArray();	//New
		//const int	numObjects = collisionPairs.size();
		//static btManifoldArray	m_manifoldArray;
		//bool added;
		//for (int i = 0; i < numObjects; i++) {
		//	const btBroadphasePair& collisionPair = collisionPairs[i];
		//	m_manifoldArray.resize(0);
		//	if (collisionPair.m_algorithm) collisionPair.m_algorithm->getAllContactManifolds(m_manifoldArray);
		//	else printf("No collisionPair.m_algorithm - probably m_dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(...) must be missing.\n");
		//	added = false;
		//	for (int j = 0; j < m_manifoldArray.size(); j++) {
		//		btPersistentManifold* manifold = m_manifoldArray[j];
		//		// Here we are in the narrowphase, but can happen that manifold->getNumContacts()==0:
		//		if (m_processOnlyObjectsWithNegativePenetrationDistance) {
		//			for (int p = 0; p < manifold->getNumContacts(); p++) {
		//				const btManifoldPoint& pt = manifold->getContactPoint(p);
		//				if (pt.getDistance() < 0.0) {
		//					// How can I be sure that the colObjs are all distinct ? I use the "added" flag.
		//					m_objectsInFrustum.push_back((btCollisionObject*)(manifold->getBody0() == m_ghostObject ? manifold->getBody1() : manifold->getBody0()));
		//					added = true;
		//					break;
		//				}
		//			}
		//			if (added) break;
		//		}
		//		else if (manifold->getNumContacts() > 0) {
		//			m_objectsInFrustum.push_back((btCollisionObject*)(manifold->getBody0() == m_ghostObject ? manifold->getBody1() : manifold->getBody0()));
		//			break;
		//		}
		//	}
		//}
	}

	BoxColliderComponent::BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD; 
		rCD.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btBoxShape(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z)));
		//btBoxShape* boxShape = (btBoxShape*)PhysicsEngine::s_physicsEngine.GetCollisionShape(rCD.collisionShapeHandle);
		//auto b = boxShape->getHalfExtentsWithMargin() * boxShape->getLocalScaling();

		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	SphereColliderComponent::SphereColliderComponent(entity entity, float radius, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD;
		rCD.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btSphereShape(radius));
		//btSphereShape* boxShape = (btSphereShape*)PhysicsEngine::s_physicsEngine.GetCollisionShape(rCD.collisionShapeHandle);
		//auto r = boxShape->getRadius();
		//auto ls = boxShape->getLocalScaling();
		//auto b = r * ls;

		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	CapsuleColliderComponent::CapsuleColliderComponent(entity entity, float radius, float height, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD;
		rCD.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btCapsuleShape(radius, height));
		//btCapsuleShape* boxShape = (btCapsuleShape*)PhysicsEngine::s_physicsEngine.GetCollisionShape(rCD.collisionShapeHandle);
		//auto r = boxShape->getRadius();
		//auto ls = boxShape->getLocalScaling();
		//auto b = r * ls;
		//auto h = 2.0f * boxShape->getHalfHeight() * ls.getY();

		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	MeshColliderComponent::MeshColliderComponent(entity entity, u32 modelID, bool drawOverride) noexcept
	{	
		meshColliderModelID = modelID;

		drawMeshColliderOverride = drawOverride;
		AssetFlags modelFlags = AssetManager::Get().GetAssetFlags(modelID);

		bool modelLoadingToCPU = modelFlags.loadFlag & AssetLoadFlag::CPUMemory;
		bool modelOnCPU = modelFlags.stateFlag & AssetStateFlag::ExistOnCPU;

		//If the model is neither being loaded to the cpu memory or does not exist in cpu memory, we assert!
		if (!(modelLoadingToCPU || modelOnCPU))
		{
			std::cout << "Asset does not have CPUMemory flag set!\nMeshColliderComponent require the mesh to be on the cpu!\n";
			assert(false);
			return;
		}

		ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelID);

		//Check if the model is loaded into memory
		if (!model)
		{
			MeshWaitData meshWaitData;
			meshWaitData.meshEntity = entity;
			meshWaitData.meshModelID = modelID;
			PhysicsEngine::AddMeshColliderWaitForModel(meshWaitData);
			return;
		}

		//The model is loaded in
		LoadMesh(entity, modelID);
	}

	void MeshColliderComponent::LoadMesh(entity entity, u32 modelID)
	{
		RigidbodyColliderData rCD;

		//Get mesh collider for an already existing mesh collider if it exists
		MeshColliderData meshColliderData = PhysicsEngine::GetMeshColliderData(modelID);

		//Set the handle, if it is zero we create a new collisionShape
		rCD.collisionShapeHandle = meshColliderData.collisionShapeHandle;

		//For the handle 0 is default value
		if (meshColliderData.collisionShapeHandle.handle == 0)
		{
			btTriangleMesh* mesh = new btTriangleMesh();
			ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelID);

			if (!model)
			{
				//Should never happen!!!
				assert(false);
			}

			struct Vertex
			{
				float x;
				float y;
				float z;
			};
			const u32 verticePerTriangle = 3;

			std::vector<u8>* vertexData = &(model->meshAsset.vertexData[VertexAttribute::Position]);
			Vertex* vertexVertices = (Vertex*)vertexData->data();

			u32 trianglesAmount = (u32)(model->meshAsset.indices.size() / verticePerTriangle);
			u32 verticesAmount = (u32)(vertexData->size() / (sizeof(Vertex)));

			//Set the mesh for the collider
			btIndexedMesh indexedMesh;
			indexedMesh.m_numTriangles = trianglesAmount;
			indexedMesh.m_triangleIndexBase = (const unsigned char*)model->meshAsset.indices.data();
			indexedMesh.m_triangleIndexStride = verticePerTriangle * sizeof(u32);
			indexedMesh.m_numVertices = verticesAmount;
			indexedMesh.m_vertexBase = (const unsigned char*)vertexVertices;
			indexedMesh.m_vertexStride = sizeof(Vertex);

			mesh->addIndexedMesh(indexedMesh);

			btCollisionShape* meshCollider = new btBvhTriangleMeshShape(mesh, true);

			//Add the mesh to the existing mesh vector
			MeshColliderData newMeshColliderData;
			newMeshColliderData.meshModelID = modelID;
			newMeshColliderData.collisionShapeHandle = PhysicsEngine::AddCollisionShape(meshCollider);
			PhysicsEngine::AddMeshColliderData(newMeshColliderData);

			rCD.collisionShapeHandle = newMeshColliderData.collisionShapeHandle;
		}

		//Meshes can not be dynamic
		//Convex meshes can be
		float mass = 0.0f;
		bool dynamic = false;
		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
		meshNotLoaded = false;
	}

	BoxTriggerComponent::BoxTriggerComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize) noexcept
	{
		GhostObjectData ghostObjectData;
		ghostObjectData.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btBoxShape(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z)));

		ghostObjectHandle = PhysicsEngine::AddGhostObject(entity, ghostObjectData);
	}
}