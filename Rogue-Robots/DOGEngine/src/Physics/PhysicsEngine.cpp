#include "PhysicsEngine.h"
#pragma warning(push, 0)
#include "BulletPhysics/btBulletDynamicsCommon.h"
#pragma warning(pop)
#include "../ECS/EntityManager.h"
#include "../Core/AssetManager.h"
#include "../Scripting/LuaMain.h"
#include "PhysicsRigidbody.h"
#include "../common/MiniProfiler.h"

using namespace DirectX::SimpleMath;

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

		//Delete ghostObjects
		for (u32 i = 0; i < m_ghostObjectDatas.size(); ++i)
		{
			btGhostObject* ghostObject = m_ghostObjectDatas[i].ghostObject;
			if (ghostObject == nullptr)
				continue;

			m_dynamicsWorld->removeCollisionObject(ghostObject);
			delete m_ghostObjectDatas[i].ghostObject;
			m_ghostObjectDatas[i].ghostObject = nullptr;
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
		MINIPROFILE
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
						//If the user updates the scale after creation
						rigidBody->rigidbodyScale = transform.GetScale();
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
						//If the user updates the scale after creation
						rigidBody->rigidbodyScale = transform.GetScale();
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
						//If the user updates the scale after creation
						rigidBody->rigidbodyScale = transform.GetScale();

						//Check if the capsule has an rigidbodycomponent and if it should have controll over the transform
						if (EntityManager::Get().HasComponent<RigidbodyComponent>(rigidBody->rigidbodyEntity))
						{
							RigidbodyComponent& rigidbodyComponent = EntityManager::Get().GetComponent<RigidbodyComponent>(rigidBody->rigidbodyEntity);
							if (rigidbodyComponent.getControlOfTransform)
							{
								btVector3 newPosition = { rigidbodyComponent.lastFramePositionDifferance.x, rigidbodyComponent.lastFramePositionDifferance.y, rigidbodyComponent.lastFramePositionDifferance.z };

								//This will cause some wrong movements sometimes, but it works!
								//Check if the distance between the rigidbody and the motion state so that they are not too far from each other
								//If they are, we set the rigidbody with the motion state world transform!
								const float maxDistance = 1.0f;
								if (abs(newPosition.getX()) > maxDistance || abs(newPosition.getY()) > maxDistance || abs(newPosition.getZ()) > maxDistance)
									rigidBody->rigidBody->setWorldTransform(trans);
								else
								{
									btTransform rigidbodyTransform;
									rigidbodyTransform = rigidBody->rigidBody->getWorldTransform();

									//Get the rotation from the world matrix
									Vector3 scale, position;
									Quaternion rotation;
									transform.worldMatrix.Decompose(scale, rotation, position);
									btQuaternion quat(rotation.x, rotation.y, rotation.z, rotation.w);
									rigidbodyTransform.setRotation(quat);

									//We add the position of the motion state transform
									newPosition += trans.getOrigin();
									rigidbodyTransform.setOrigin(newPosition);

									rigidBody->rigidBody->setWorldTransform(rigidbodyTransform);
								}
							}
						}
					}
				});

			//Ghost objects have no physics done to them but we want to update the boxtriggers position in the physics world
			EntityManager::Get().Collect<TransformComponent, BoxTriggerComponent>().Do([&](TransformComponent& transform, BoxTriggerComponent& trigger)
				{
					//Get ghostObject
					auto* ghostObjectData = s_physicsEngine.GetGhostObjectData(trigger.ghostObjectHandle);
					if (ghostObjectData->ghostObject)
					{
						btTransform trans;
						trans.setFromOpenGLMatrix((float*)(&transform.worldMatrix));
						ghostObjectData->ghostObject->setWorldTransform(trans);
					}
				});

			//Ghost objects have no physics done to them but we want to update the spheretriggers position in the physics world
			EntityManager::Get().Collect<TransformComponent, SphereTriggerComponent>().Do([&](TransformComponent& transform, SphereTriggerComponent& trigger)
				{
					//Get ghostObject
					auto* ghostObjectData = s_physicsEngine.GetGhostObjectData(trigger.ghostObjectHandle);
					if (ghostObjectData->ghostObject)
					{
						btTransform trans;
						trans.setFromOpenGLMatrix((float*)(&transform.worldMatrix));
						ghostObjectData->ghostObject->setWorldTransform(trans);
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

					//Check if the capsule has an rigidbodycomponent and if it should have controll over the transform
					if (EntityManager::Get().HasComponent<RigidbodyComponent>(rigidBody->rigidbodyEntity))
					{
						RigidbodyComponent& rigidbodyComponent = EntityManager::Get().GetComponent<RigidbodyComponent>(rigidBody->rigidbodyEntity);
						if (rigidbodyComponent.getControlOfTransform)
						{
							btTransform rigidbodyTransform;
							rigidbodyTransform = rigidBody->rigidBody->getWorldTransform();
							btVector3 difference = rigidbodyTransform.getOrigin() - trans.getOrigin();
							//Because the motion state and rigidbody have different positions sometimes we need this for it to work properly
							rigidbodyComponent.lastFramePositionDifferance = { difference.getX(), difference.getY(), difference.getZ() };
						}
					}
				}
			});

		s_physicsEngine.CheckRigidbodyCollisions();

		//Because for now we do not have any callbacks to c++ we only have to check if entity have a ScriptComponent
		EntityManager::Get().Collect<RigidbodyComponent, ScriptComponent>().Do([&](entity obj0Entity, RigidbodyComponent& rigidbody, ScriptComponent&)
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
							entity obj1Entity = 0;
							if (!it->second.ghost)
								obj1Entity = PhysicsEngine::GetRigidbodyColliderData(it->second.collisionBodyHandle)->rigidbodyEntity;
							else
								obj1Entity = PhysicsEngine::GetGhostObjectData((GhostObjectHandle)(it->second.collisionBodyHandle.handle))->ghostObjectEntity;

							//Fix later
							//if (it->second.activeCollision && rigidbody.onCollisionEnter != nullptr)
							//	rigidbody.onCollisionEnter(obj0RigidbodyColliderData->rigidbodyEntity, obj1RigidbodyColliderData->rigidbodyEntity);
							if (it->second.activeCollision)
								LuaMain::GetScriptManager()->CallFunctionOnAllEntityScripts(obj0Entity, "OnCollisionEnter", obj1Entity);

							//Set collisionCheck false for next collision check
							it->second.collisionCheck = false;

							if (!it->second.activeCollision)
							{
								//Fix later
								//if (rigidbody.onCollisionExit != nullptr)
								//	rigidbody.onCollisionExit(obj0RigidbodyColliderData->rigidbodyEntity, obj1RigidbodyColliderData->rigidbodyEntity);
								//else
								LuaMain::GetScriptManager()->CallFunctionOnAllEntityScripts(obj0Entity, "OnCollisionExit", obj1Entity);

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
		if (EntityManager::Get().HasComponent<BoxTriggerComponent>(entity))
		{
			BoxTriggerComponent& colliderComponent = EntityManager::Get().GetComponent<BoxTriggerComponent>(entity);
			s_physicsEngine.FreeGhostObjectData(colliderComponent.ghostObjectHandle);
		}
		if (EntityManager::Get().HasComponent<SphereTriggerComponent>(entity))
		{
			SphereTriggerComponent& colliderComponent = EntityManager::Get().GetComponent<SphereTriggerComponent>(entity);
			s_physicsEngine.FreeGhostObjectData(colliderComponent.ghostObjectHandle);
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

	void PhysicsEngine::FreePhysicsFromDeferredEntities()
	{
		//Destroy all of the entities with the deferred deletion flag set
		EntityManager::Get().Collect<DeferredDeletionComponent>().Do([&](entity entity, DeferredDeletionComponent&)
			{
				PhysicsEngine::FreePhysicsFromEntity(entity);
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
		const bool isRigidbody = true;
		u32 userIndex1 = (u32)(rigidbodyHandle.handle & bitMask);
		u32 userIndex2 = (u32)((rigidbodyHandle.handle >> byteShift) & bitMask);
		pointerToRCD->rigidBody->setUserIndex(userIndex1);
		pointerToRCD->rigidBody->setUserIndex2(userIndex2);
		//Set if the collision object is a rigidbody or a ghost
		//In this case it is a rigidbody
		pointerToRCD->rigidBody->setUserIndex3(isRigidbody);

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

		return ghostObjectHandle;
	}

	GhostObjectHandle PhysicsEngine::AddGhostObject(entity entity, GhostObjectData& ghostObjectData)
	{
		//Set ghost object entity
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
		const bool isRigidbody = false;
		u32 userIndex1 = (u32)(ghostObjectHandle.handle & bitMask);
		u32 userIndex2 = (u32)((ghostObjectHandle.handle >> byteShift) & bitMask);
		ghostObjectData.ghostObject->setUserIndex(userIndex1);
		ghostObjectData.ghostObject->setUserIndex2(userIndex2);
		//Set if the collision object is a rigidbody or a ghost
		//In this case it is a ghost
		ghostObjectData.ghostObject->setUserIndex3(isRigidbody);

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
				MeshColliderComponent& component = EntityManager::Get().GetComponent<MeshColliderComponent>(m_meshCollidersWaitingForModels[index].meshEntity);
				component.LoadMesh(m_meshCollidersWaitingForModels[index].meshEntity, m_meshCollidersWaitingForModels[index].meshModelID, 
					m_meshCollidersWaitingForModels[index].localMeshScale);
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

	void PhysicsEngine::FreeGhostObjectData(const GhostObjectHandle& ghostObjectHandle)
	{
		GhostObjectData* ghostObjectData = GetGhostObjectData(ghostObjectHandle);

		m_dynamicsWorld->removeCollisionObject(ghostObjectData->ghostObject);
		delete ghostObjectData->ghostObject;
		ghostObjectData->ghostObject = nullptr;

		FreeCollisionShape(ghostObjectData->collisionShapeHandle);

		m_handleAllocator.Free(ghostObjectHandle);
	}

	void PhysicsEngine::CheckRigidbodyCollisions()
	{
		int numManifolds = s_physicsEngine.m_dynamicsWorld->getDispatcher()->getNumManifolds();
		for (int i = 0; i < numManifolds; i++)
		{
			btPersistentManifold* contactManifold = s_physicsEngine.m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
			btCollisionObject* obj0 = (btCollisionObject*)(contactManifold->getBody0());
			btCollisionObject* obj1 = (btCollisionObject*)(contactManifold->getBody1());

			for (int j = 0; j < contactManifold->getNumContacts(); j++)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(j);
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

					//This can be either a rigidbody or a ghost
					//Get obj1 collisionbody handle
					u64 obj1CollisionHandle = (obj1->getUserIndex2() << byteShift) | obj1->getUserIndex();
					//Get handle for obj1
					u32 obj1Handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(obj1CollisionHandle);

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
						//Ghost objects can also enter here!
						collisionData.ghost = !obj1->getUserIndex3();
						//This is little bit sus but RigidbodyHandle and GhostObjectHandle only have u64 in them
						RigidbodyHandle handle;
						handle.handle = obj1CollisionHandle;
						collisionData.collisionBodyHandle = handle;
						collisions->second.insert({ obj1Handle, collisionData });
					}

					//If the two objects did collide then we do not need to check the other collision points
					break;
				}
			}
		}
	}

	BoxColliderComponent::BoxColliderComponent(entity entity, const Vector3& boxColliderSize, bool dynamic, float mass) noexcept
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

	MeshColliderComponent::MeshColliderComponent(entity entity, u32 modelID, const Vector3& localMeshScale, bool drawOverride) noexcept
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
			meshWaitData.localMeshScale = localMeshScale;
			PhysicsEngine::AddMeshColliderWaitForModel(meshWaitData);
			return;
		}

		//The model is loaded in
		LoadMesh(entity, modelID, localMeshScale);
	}

	void MeshColliderComponent::LoadMesh(entity entity, u32 modelID, const Vector3& localMeshScale)
	{
		RigidbodyColliderData rCD;

		//Get mesh collider for an already existing mesh collider if it exists
		MeshColliderData meshColliderData = PhysicsEngine::GetMeshColliderData(modelID);

		//Set the handle, if it is zero we create a new collisionShape
		rCD.collisionShapeHandle = meshColliderData.collisionShapeHandle;

		//For the handle 0 is default value
		if (meshColliderData.collisionShapeHandle.handle != 0)
		{
			//We check if the existing scaledMeshCollider has the same scale of the incoming meshcollider
			//If it does not we create a new scaledMeshCollider with the scale requested
			btScaledBvhTriangleMeshShape* scaledMeshCollider = (btScaledBvhTriangleMeshShape*)PhysicsEngine::s_physicsEngine.GetCollisionShape(rCD.collisionShapeHandle);

			if (btVector3(localMeshScale.x, localMeshScale.y, localMeshScale.z) != scaledMeshCollider->getLocalScaling())
			{
				//The new scaledMeshCollider uses the mesh collider of the old scaledMeshCollider, so we do not create a new meshCollider (we save nemory)
				btScaledBvhTriangleMeshShape* newScaledMeshCollider = new btScaledBvhTriangleMeshShape(scaledMeshCollider->getChildShape(), btVector3(localMeshScale.x, localMeshScale.y, localMeshScale.z));

				//Add the scaled mesh to the existing mesh vector
				MeshColliderData newMeshColliderData;
				newMeshColliderData.meshModelID = modelID;
				newMeshColliderData.collisionShapeHandle = PhysicsEngine::AddCollisionShape(newScaledMeshCollider);
				PhysicsEngine::AddMeshColliderData(newMeshColliderData);

				rCD.collisionShapeHandle = newMeshColliderData.collisionShapeHandle;
			}
		}
		//We load in a new mesh as a mesh collider and then we reuse it for other mesh colliders who uses the same model
		else
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

			btBvhTriangleMeshShape* meshCollider = new btBvhTriangleMeshShape(mesh, true);

			//Create a mesh collider which we can scale! (this is needed for the flipped models)
			btScaledBvhTriangleMeshShape* scaledMeshCollider = new btScaledBvhTriangleMeshShape(meshCollider, btVector3(localMeshScale.x, localMeshScale.y, localMeshScale.z));

			//Add the mesh to the existing mesh vector
			MeshColliderData newMeshColliderData;
			newMeshColliderData.meshModelID = modelID;
			newMeshColliderData.collisionShapeHandle = PhysicsEngine::AddCollisionShape(scaledMeshCollider);
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

	BoxTriggerComponent::BoxTriggerComponent(entity entity, const Vector3& boxColliderSize) noexcept
	{
		GhostObjectData ghostObjectData;
		ghostObjectData.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btBoxShape(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z)));

		ghostObjectHandle = PhysicsEngine::AddGhostObject(entity, ghostObjectData);
	}

	SphereTriggerComponent::SphereTriggerComponent(entity entity, float radius) noexcept
	{
		GhostObjectData ghostObjectData;
		ghostObjectData.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btSphereShape(radius));

		ghostObjectHandle = PhysicsEngine::AddGhostObject(entity, ghostObjectData);
	}
}