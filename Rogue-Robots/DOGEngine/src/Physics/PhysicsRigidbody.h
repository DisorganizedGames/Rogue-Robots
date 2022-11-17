#pragma once
#include "PhysicsEngine.h"

namespace DOG
{
	struct RigidbodyComponent
	{
		RigidbodyComponent(entity entity, bool kinematicBody = false);

		void ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation);
		void ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition);

		RigidbodyHandle rigidbodyHandle;

		bool constrainRotationX, constrainRotationY, constrainRotationZ;
		bool constrainPositionX, constrainPositionY, constrainPositionZ;
		DirectX::SimpleMath::Vector3 linearVelocity;
		DirectX::SimpleMath::Vector3 centralForce;
		DirectX::SimpleMath::Vector3 centralImpulse;
		DirectX::SimpleMath::Vector3 torque;
		DirectX::SimpleMath::Vector3 angularVelocity;
		float mass = 1.0f;
		bool disableDeactivation = false;
		bool noCollisionResponse = false;

		//Continuous Collision Detection (used for fast moving objects like bullets)
		bool continuousCollisionDetection = false;
		float continuousCollisionDetectionMotionThreshold = (float)1e-7;
		float continuousCollisionDetectionSweptSphereRadius = 0.2f;

		//With this you can rotate and change the position of the transform directly (do not change the transform every frame as this causes the physics to not work properly)
		bool getControlOfTransform = false;
		DirectX::SimpleMath::Vector3 lastFramePositionDifferance;
	};

	class PhysicsRigidbody
	{
	public:
		static void UpdateRigidbodies();
		static void UpdateValuesForRigidbodies();
	};
}