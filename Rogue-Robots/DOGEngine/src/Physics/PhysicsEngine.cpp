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
	}

	void PhysicsEngine::AddMeshColliderWaitForModel(MeshWaitData meshColliderData)
	{
		s_physicsEngine.m_meshCollidersWaitingForModels.push_back(meshColliderData);
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
		s_physicsEngine.CheckMeshColliders();

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

	void PhysicsEngine::CheckMeshColliders()
	{
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

		btTriangleMesh* mesh = new btTriangleMesh();
		ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelID);

		struct Vertex
		{
			float x;
			float y;
			float z;
		};
		std::vector<u8>* vertexData = &(model->meshAsset.vertexData[VertexAttribute::Position]);
		Vertex* vertexVertices = (Vertex*)vertexData->data();
		u32 trianglesAmount = model->meshAsset.indices.size() / 3;
		u32 verticesAmount = vertexData->size() / (sizeof(Vertex));
		//for (u32 i = 0; i < model->meshAsset.indices.size(); i += 3)
		//{
		//	u32 index1 = model->meshAsset.indices[i];
		//	u32 index2 = model->meshAsset.indices[i+1];
		//	u32 index3 = model->meshAsset.indices[i+2];
		//	//Vertex vertex = ;
		//	Vertex vertex1 = vertexVertices[index1];
		//	Vertex vertex2 = vertexVertices[index2];
		//	Vertex vertex3 = vertexVertices[index3];

		//	mesh->addTriangle(btVector3(vertex1.x,vertex1.y,vertex1.z), btVector3(vertex2.x, vertex2.y, vertex2.z), btVector3(vertex3.x, vertex3.y, vertex3.z));
		//}

		btIndexedMesh indexedMesh;
		indexedMesh.m_numTriangles = trianglesAmount;
		indexedMesh.m_triangleIndexBase = (const unsigned char*)model->meshAsset.indices.data();
		indexedMesh.m_triangleIndexStride = 3 * sizeof(u32);// 3 * sizeof(float) * 3;
		indexedMesh.m_numVertices = verticesAmount;
		indexedMesh.m_vertexBase = (const unsigned char*)vertexVertices;
		indexedMesh.m_vertexStride = sizeof(Vertex);
		mesh->addIndexedMesh(indexedMesh);

		rCD.collisionShape = new btBvhTriangleMeshShape(mesh, true);

		//auto numberOfTriangles = model->meshAsset.indices.size() / 3;
		//btIndexedMesh indexedMesh;
		//btTriangleIndexVertexArray* g = new btTriangleIndexVertexArray();
		//indexedMesh.m_numTriangles = numberOfTriangles;
		//indexedMesh.m_triangleIndexBase = (const unsigned char*)model->meshAsset.indices.data();
		//indexedMesh.m_triangleIndexStride = 0;
		//indexedMesh.m_numVertices = model->meshAsset.indices.size();
		//indexedMesh.m_vertexBase = (const unsigned char*)vertices;
		//indexedMesh.m_vertexStride = 3 * sizeof(float);

		//g->addIndexedMesh(indexedMesh);
		//rCD.collisionShape = new btBvhTriangleMeshShape(g, true);


		float mass = 0.0f;
		bool dynamic = false;
		handle = PhysicsEngine::AddRigidbody(entity, rCD, dynamic, mass);
		meshNotLoaded = false;
	}
}