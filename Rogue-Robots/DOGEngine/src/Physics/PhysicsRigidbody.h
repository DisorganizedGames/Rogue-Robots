#pragma once
#include "PhysicsEngine.h"

namespace DOG
{
	struct RigidbodyComponent
	{
		RigidbodyComponent(entity entity);

		void SetOnCollisionEnter(std::function<void(entity, entity)> onCollisionEnter);
		void SetOnCollisionExit(std::function<void(entity, entity)> onCollisionExit);
		void ConstrainRotation(bool constrainXRotation, bool constrainYRotation, bool constrainZRotation);
		void ConstrainPosition(bool constrainXPosition, bool constrainYPosition, bool constrainZPosition);

		RigidbodyHandle rigidbodyHandle;
		std::function<void(entity, entity)> onCollisionEnter = nullptr;
		std::function<void(entity, entity)> onCollisionExit = nullptr;

		bool constrainRotation[3];
		bool constrainPosition[3];
		DirectX::SimpleMath::Vector3 linearVelocity;
		DirectX::SimpleMath::Vector3 centralForce;
		float mass = 1.0f;
	};

	class PhysicsRigidbody
	{
	public:
		static void UpdateRigidbodies();
	};
}