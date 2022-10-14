#pragma once
#include "PhysicsEngine.h"

namespace DOG
{
	struct RigidbodyComponent
	{
		RigidbodyComponent(entity entity, bool kinematicBody = false);

		//Fix later
		//void SetOnCollisionEnter(std::function<void(entity, entity)> onCollisionEnter);
		//void SetOnCollisionExit(std::function<void(entity, entity)> onCollisionExit);
		void ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation);
		void ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition);

		RigidbodyHandle rigidbodyHandle;
		//Fix later
		//std::function<void(entity, entity)> onCollisionEnter = nullptr;
		//std::function<void(entity, entity)> onCollisionExit = nullptr;

		bool constrainRotationX, constrainRotationY, constrainRotationZ;
		bool constrainPositionX, constrainPositionY, constrainPositionZ;
		DirectX::SimpleMath::Vector3 linearVelocity;
		DirectX::SimpleMath::Vector3 centralForce;
		float mass = 1.0f;
		bool disableDeactivation = false;

		//Continuous Collision Detection (used for fast moving objects like bullets)
		bool continuousCollisionDetection = false;
		float continuousCollisionDetectionMotionThreshold = (float)1e-7;
		float continuousCollisionDetectionSweptSphereRadius = 0.2f;
	};

	class PhysicsRigidbody
	{
	public:
		static void UpdateRigidbodies();
		static void UpdateValuesForRigidbodies();
	};
}