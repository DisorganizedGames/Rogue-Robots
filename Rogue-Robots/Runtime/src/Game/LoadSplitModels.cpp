#include "LoadSplitModels.h"

#include "DirectXMath.h"
#include <DirectXTK/SimpleMath.h>


using namespace DOG;
using Vector3 = DirectX::SimpleMath::Vector3;
using Matrix = DirectX::SimpleMath::Matrix;

void LoadEnemySplitModel(entity agentID, SceneComponent::Type scene)
{
	const f64 aliveTime = 120.0f;

	EntityManager& em = EntityManager::Get();

	TransformComponent& agentTrans = em.GetComponent<TransformComponent>(agentID);

	//Set up main body of the enemy
	u32 bodyID = AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/SplitUpEnemy/Body.gltf", DOG::AssetLoadFlag::GPUMemory);
	Vector3 bodyOffset = Vector3(-0.000064f, 0.511343f, 0.135723f);
	entity corpseBody = em.CreateEntity();

	TransformComponent& bodyTrans = em.AddComponent<TransformComponent>(corpseBody);
	bodyTrans.worldMatrix = agentTrans.worldMatrix;
	bodyOffset = Vector3::Transform(bodyOffset, bodyTrans.GetRotation());
	bodyTrans.SetPosition(bodyTrans.GetPosition() + bodyOffset);

	em.AddComponent<ModelComponent>(corpseBody, bodyID);
	em.AddComponent<BoxColliderComponent>(corpseBody, corpseBody, Vector3(0.3f, 0.2f, 0.5f), true);
	em.AddComponent<RigidbodyComponent>(corpseBody, corpseBody);
	em.AddComponent<DespawnComponent>(corpseBody, aliveTime);
	em.AddComponent<SceneComponent>(corpseBody, scene);

	//Set up the tail of the enemy
	u32 tailID = AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/SplitUpEnemy/Tail.gltf", DOG::AssetLoadFlag::GPUMemory);
	Vector3 tailOffset = Vector3(0.0f, 0.801857f, 0.627901f);
	entity corpseTail = em.CreateEntity();

	TransformComponent& trailTrans = em.AddComponent<TransformComponent>(corpseTail);
	trailTrans.worldMatrix = agentTrans.worldMatrix;
	tailOffset = Vector3::Transform(tailOffset, trailTrans.GetRotation());
	trailTrans.SetPosition(trailTrans.GetPosition() + tailOffset);

	em.AddComponent<ModelComponent>(corpseTail, tailID);
	em.AddComponent<BoxColliderComponent>(corpseTail, corpseTail, Vector3(0.07f, 0.3f, 0.3f), true);
	em.AddComponent<RigidbodyComponent>(corpseTail, corpseTail);
	em.AddComponent<DespawnComponent>(corpseTail, aliveTime);
	em.AddComponent<SceneComponent>(corpseTail, scene);

	//Set up the legs of the enemy
	u32 legID = AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/SplitUpEnemy/Leg1.gltf", DOG::AssetLoadFlag::GPUMemory);
	Vector3 legOffset = Vector3(0.373457f, 0.293323f, 0.000132f);
	const u32 legsAmount = 6;
	const u32 legsOnEachSide = 3;
	const f32 legsDistanceFromEachOther = 0.20168f;

	for (int i = 0; i < legsAmount; ++i)
	{
		entity corpseLeg = em.CreateEntity();
		TransformComponent& legTrans = em.AddComponent<TransformComponent>(corpseLeg);
		legTrans.worldMatrix = agentTrans.worldMatrix;

		legTrans.SetPosition(legTrans.GetPosition() + legTrans.GetForward() * float(i % legsOnEachSide) * legsDistanceFromEachOther);

		Vector3 offset = Vector3::Transform(legOffset, legTrans.GetRotation());

		if (i >= legsOnEachSide)
		{
			//"Flip" the model
			legTrans.RotateL(Vector3(0.0f, DirectX::XM_PI, 0.0f));
			//Rotate the offset to be at correct position for the "flipped" models
			offset = Vector3::Transform(offset, Matrix::CreateFromAxisAngle(agentTrans.GetUp(), DirectX::XM_PI));
		}

		legTrans.SetPosition(legTrans.GetPosition() + offset);

		em.AddComponent<ModelComponent>(corpseLeg, legID);
		em.AddComponent<BoxColliderComponent>(corpseLeg, corpseLeg, Vector3(0.35f, 0.3f, 0.05f), true);
		em.AddComponent<RigidbodyComponent>(corpseLeg, corpseLeg);
		em.AddComponent<DespawnComponent>(corpseLeg, aliveTime);
		em.AddComponent<SceneComponent>(corpseLeg, scene);
	}
}