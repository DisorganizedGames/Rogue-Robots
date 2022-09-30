#include "BulletPhysics.h"
#include "BulletPhysics/btBulletDynamicsCommon.h"
#include "../ECS/EntityManager.h"

namespace DOG
{
	BulletPhysics BulletPhysics::s_bulletPhysics;

	BulletPhysics::BulletPhysics()
	{

	}

	btDiscreteDynamicsWorld* BulletPhysics::GetDynamicsWorld()
	{
		return s_bulletPhysics.m_dynamicsWorld.get();
	}

	BulletPhysics::~BulletPhysics()
	{
		m_rigidBodyColliderDatas.clear();
	}

	void BulletPhysics::Initialize()
	{
		s_bulletPhysics.m_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
		s_bulletPhysics.m_collisionDispatcher = std::make_unique<btCollisionDispatcher>(s_bulletPhysics.m_collisionConfiguration.get());
		s_bulletPhysics.m_broadphaseInterface = std::make_unique<btDbvtBroadphase>();
		s_bulletPhysics.m_sequentialImpulseContraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
		s_bulletPhysics.m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(s_bulletPhysics.m_collisionDispatcher.get(),
			s_bulletPhysics.m_broadphaseInterface.get(), s_bulletPhysics.m_sequentialImpulseContraintSolver.get(), s_bulletPhysics.m_collisionConfiguration.get());

		s_bulletPhysics.m_dynamicsWorld->setGravity({0.0f, -9.82f, 0.0f});
	}

	void BulletPhysics::BulletTest()
	{
		std::cout << "Hello World\n";

		///-----includes_end-----

		int i;
		///-----initialization_start-----

		/////collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		//btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();

		/////use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		//btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

		/////btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		//btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();

		/////the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		//btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

		//btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

		s_bulletPhysics.m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

		///-----initialization_end-----

		//keep track of the shapes, we release memory at exit.
		//make sure to re-use collision shapes among rigid bodies whenever possible!
		btAlignedObjectArray<btCollisionShape*> collisionShapes;

		///create a few basic rigid bodies

		//the ground is a cube of side 100 at position y = -56.
		//the sphere will hit it at y = -6, with center at -5
		{
			btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

			collisionShapes.push_back(groundShape);

			btTransform groundTransform;
			groundTransform.setIdentity();
			groundTransform.setOrigin(btVector3(0, -56, 0));

			btScalar mass(0.);

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
				groundShape->calculateLocalInertia(mass, localInertia);

			//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
			btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
			btRigidBody* body = new btRigidBody(rbInfo);

			//add the body to the dynamics world
			s_bulletPhysics.m_dynamicsWorld->addRigidBody(body);
		}

		{
			//create a dynamic rigidbody

			//btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
			btCollisionShape* colShape = new btSphereShape(btScalar(1.));
			collisionShapes.push_back(colShape);

			/// Create Dynamic Objects
			btTransform startTransform;
			startTransform.setIdentity();

			btScalar mass(1.f);

			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
				colShape->calculateLocalInertia(mass, localInertia);

			startTransform.setOrigin(btVector3(2, 10, 0));

			//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
			btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
			btRigidBody* body = new btRigidBody(rbInfo);

			s_bulletPhysics.m_dynamicsWorld->addRigidBody(body);
		}

		/// Do some simulation

		///-----stepsimulation_start-----
		for (i = 0; i < 150; i++)
		{
			s_bulletPhysics.m_dynamicsWorld->stepSimulation(1.f / 60.f, 10);

			//print positions of all objects
			for (int j = s_bulletPhysics.m_dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--)
			{
				btCollisionObject* obj = s_bulletPhysics.m_dynamicsWorld->getCollisionObjectArray()[j];
				btRigidBody* body = btRigidBody::upcast(obj);
				if (i < 10)
					body->setLinearVelocity(btVector3(0, 10, 0));
				auto velocity = body->getLinearVelocity();
				btTransform trans;
				if (body && body->getMotionState())
				{
					body->getMotionState()->getWorldTransform(trans);
				}
				else
				{
					trans = obj->getWorldTransform();
				}
				printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
			}
		}

		///-----stepsimulation_end-----

		//cleanup in the reverse order of creation/initialization

		///-----cleanup_start-----

		//remove the rigidbodies from the dynamics world and delete them
		for (i = s_bulletPhysics.m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = s_bulletPhysics.m_dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}
			s_bulletPhysics.m_dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}

		//delete collision shapes
		for (int j = 0; j < collisionShapes.size(); j++)
		{
			btCollisionShape* shape = collisionShapes[j];
			collisionShapes[j] = 0;
			delete shape;
		}

		////delete dynamics world
		//delete dynamicsWorld;

		////delete solver
		//delete solver;

		////delete broadphase
		//delete overlappingPairCache;

		////delete dispatcher
		//delete dispatcher;

		//delete collisionConfiguration;

		//next line is optional: it will be cleared by the destructor when the array goes out of scope
		collisionShapes.clear();
	}

	void BulletPhysics::UpdatePhysics(float deltaTime)
	{
		s_bulletPhysics.GetDynamicsWorld()->stepSimulation(deltaTime, 10);

		EntityManager::Get().Collect<TransformComponent, BoxColliderComponent>().Do([&](TransformComponent& transform, BoxColliderComponent& boxCollider)
			{
				auto& rigidBody = s_bulletPhysics.m_rigidBodyColliderDatas[boxCollider.handle];
				if (rigidBody.rigidBody.get() && rigidBody.rigidBody->getMotionState())
				{
					btTransform trans;
					rigidBody.rigidBody->getMotionState()->getWorldTransform(trans);
					trans.getOpenGLMatrix((float*)(&transform.worldMatrix));
				}
			});
	}

	u32 BulletPhysics::AddRigidbodyColliderData(RigidbodyColliderData rigidbodyColliderData)
	{
		s_bulletPhysics.m_rigidBodyColliderDatas.push_back(std::move(rigidbodyColliderData));
		return s_bulletPhysics.m_rigidBodyColliderDatas.size() - 1;
	}

	BoxColliderComponent::BoxColliderComponent(entity entity, const DirectX::SimpleMath::Vector3& boxColliderSize, bool dynamic, float mass) noexcept
	{
		RigidbodyColliderData rCD; 
		rCD.collisionShape = std::make_unique<btBoxShape>(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z));

		//btCollisionShape* groundShape = new btBoxShape(btVector3(boxColliderSize.x, boxColliderSize.y, boxColliderSize.z));

		TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(entity);

		//Copy entity transform
		btTransform groundTransform;
		groundTransform.setFromOpenGLMatrix((float*)(&transform.worldMatrix));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = dynamic;//(mass != 0.f);
		float bodyMass = 0.0f;

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
		{
			rCD.collisionShape->calculateLocalInertia(mass, localInertia);
			bodyMass = mass;
		}

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		rCD.motionState = std::make_unique<btDefaultMotionState>(groundTransform);
		//btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(bodyMass, rCD.motionState.get(), rCD.collisionShape.get(), localInertia);
		rCD.rigidBody = std::make_unique<btRigidBody>(rbInfo);
		/*rigidBody = new btRigidBody(rbInfo);*/ //std::make_unique<btRigidBody>(rbInfo);
		//btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		BulletPhysics::GetDynamicsWorld()->addRigidBody(rCD.rigidBody.get());
		handle = BulletPhysics::AddRigidbodyColliderData(std::move(rCD));
	}
}