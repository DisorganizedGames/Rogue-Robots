#pragma once
#include <DOGEngine.h>

#include "../DOGEngine/src/Core/DataPiper.h"
#include "DebugCamera.h"


struct PositionComponent
{
	Vector3f position;
	u32 id = 0u;
};

struct VelocityComponent
{
	Vector3f velocity;
	u32 id = 1u;
};

constexpr const u32 ENTITY_CAPACITY = 64'000;
typedef u32 entity;
#define NULL_ENTITY ENTITY_CAPACITY

struct SparseSetBase
{

};

template<typename ComponentType>
struct SparseSet : public SparseSetBase
{
	std::vector<entity> sparseArray;
	std::vector<entity> denseArray;
	std::vector<ComponentType> components;
};

class EntityManager
{
public:

	std::vector<std::unique_ptr<SparseSetBase>> components;
private:
};

class GameLayer : public DOG::Layer
{
public:
	GameLayer() noexcept;
	virtual ~GameLayer() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;
	void AddC();
	void RemoveC();
	void HasC();
	void AddComponent(entity entityID, const PositionComponent& component) noexcept;
	void RemoveComponent(entity entityID);
	bool HasComponent(entity entityID);
	void AddEntity();
	void PrintSparseSet();
private:
	DOG::piper::PipedData m_pipedData{};

private:
	DebugCamera m_debugCam;
	u64 m_redCube{ 0 };
	u64 m_greenCube{ 0 };
	u64 m_blueCube{ 0 };
	u64 m_magentaCube{ 0 };
	u32 nextEntity = 0u;
	std::vector<entity> entities;
	EntityManager mgr;
};