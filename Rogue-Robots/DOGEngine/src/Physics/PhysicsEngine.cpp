#include "PhysicsEngine.h"
#pragma warning(push, 0)
#include "BulletPhysics/btBulletDynamicsCommon.h"
#pragma warning(pop)
#include "../ECS/EntityManager.h"
#include "../Core/AssetManager.h"

namespace DOG
{
	PhysicsEngine PhysicsEngine::s_physicsEngine;

	PhysicsEngine::PhysicsEngine()
	{
		m_rigidBodyColliderDatas.resize(PhysicsEngine::RESIZE_RIGIDBODY_SIZE);

		m_collisionShapes.resize(PhysicsEngine::RESIZE_COLLISIONSHAPE_SIZE);
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
	}

	void PhysicsEngine::UpdatePhysics(float deltaTime)
	{
		s_physicsEngine.CheckMeshColliders();

		s_physicsEngine.GetDynamicsWorld()->stepSimulation(deltaTime, 10);

		EntityManager::Get().Collect<TransformComponent, RigidbodyComponent>().Do([&](TransformComponent& transform, RigidbodyComponent& rigidbody)
			{
				//Get handle for vector
				u32 handle = s_physicsEngine.m_handleAllocator.GetSlot(rigidbody.rigidbodyHandle.handle);

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

		//add the body to the dynamics world
		PhysicsEngine::GetDynamicsWorld()->addRigidBody(rigidbodyColliderData.rigidBody);

		return PhysicsEngine::AddRigidbodyColliderData(std::move(rigidbodyColliderData));
	}

	RigidbodyColliderData* PhysicsEngine::GetRigidbodyColliderData(u32 handle)
	{
		return &s_physicsEngine.m_rigidBodyColliderDatas[handle];
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
		u32 handle = gfx::HandleAllocator::GetSlot(collisionShapeHandle.handle);

		//Resize if needed
		if (handle >= s_physicsEngine.m_collisionShapes.size())
			s_physicsEngine.m_collisionShapes.resize(s_physicsEngine.m_collisionShapes.size() + PhysicsEngine::RESIZE_COLLISIONSHAPE_SIZE);

		s_physicsEngine.m_collisionShapes[handle] = addCollisionShape;

		return collisionShapeHandle;
	}

	btCollisionShape* PhysicsEngine::GetCollisionShape(const CollisionShapeHandle& collisionShapeHandle)
	{
		u32 handle = gfx::HandleAllocator::GetSlot(collisionShapeHandle.handle);

		//0 is default for HandleAllocator
		if (handle == 0)
		{
			std::cout << "Handle does not exist!\n";
			assert(false);
		}

		return m_collisionShapes[handle];
	}

	BoxColliderComponent::BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD; 
		rCD.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btBoxShape(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z)));

		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	SphereColliderComponent::SphereColliderComponent(entity entity, float radius, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD;
		rCD.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btSphereShape(radius));

		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	CapsuleColliderComponent::CapsuleColliderComponent(entity entity, float radius, float height, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD;
		rCD.collisionShapeHandle = PhysicsEngine::AddCollisionShape(new btCapsuleShape(radius, height));

		rigidbodyHandle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
	}

	RigidbodyComponent::RigidbodyComponent(entity enitity)
	{
		//Can only create a rigidbody component for box, sphere, capsule
		if (EntityManager::Get().HasComponent<BoxColliderComponent>(enitity))
		{
			rigidbodyHandle = EntityManager::Get().GetComponent<BoxColliderComponent>(enitity).rigidbodyHandle;
		}
		else if (EntityManager::Get().HasComponent<SphereColliderComponent>(enitity))
		{
			rigidbodyHandle = EntityManager::Get().GetComponent<SphereColliderComponent>(enitity).rigidbodyHandle;
		}
		else if (EntityManager::Get().HasComponent<CapsuleColliderComponent>(enitity))
		{
			rigidbodyHandle = EntityManager::Get().GetComponent<CapsuleColliderComponent>(enitity).rigidbodyHandle;
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
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(rigidbodyHandle.handle);

		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(handle);

		//Set no rotations in x,y,z
		float x = constrainXRotation ? 0.0f : 1.0f;
		float y = constrainYRotation ? 0.0f : 1.0f;
		float z = constrainZRotation ? 0.0f : 1.0f;

		rigidbodyColliderData->rigidBody->setAngularFactor(btVector3(x, y, z));
	}

	void RigidbodyComponent::ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition)
	{
		//Get handle for vector
		u32 handle = PhysicsEngine::s_physicsEngine.m_handleAllocator.GetSlot(rigidbodyHandle.handle);

		RigidbodyColliderData* rigidbodyColliderData = PhysicsEngine::GetRigidbodyColliderData(handle);

		////Set freeze position in x,y,z
		float x = constrainXPosition ? 0.0f : 1.0f;
		float y = constrainYPosition ? 0.0f : 1.0f;
		float z = constrainZPosition ? 0.0f : 1.0f;

		rigidbodyColliderData->rigidBody->setLinearFactor(btVector3(x, y, z));
	}

	MeshColliderComponent::MeshColliderComponent(entity entity, u32 modelID) noexcept
	{	
		AssetLoadFlag modelLoadFlag = AssetManager::Get().GetAssetFlags(modelID);
		if (!(modelLoadFlag & AssetLoadFlag::CPUMemory))
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
}