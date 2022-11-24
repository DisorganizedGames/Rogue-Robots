#pragma once
#include <DOGEngine.h>
//#include "NavMesh.h"
//#include "Portal.h"
#include "../Game/GameComponent.h"
#include "PathfinderComponents.h"

class Pathfinder
{
using Vector3 = DirectX::SimpleMath::Vector3;
public:
	using NavMeshID = DOG::entity;
	using PortalID = DOG::entity;

	// public methods
	[[nodiscard]] static constexpr Pathfinder& Get() noexcept { return s_instance; }

	void BuildNavScene(SceneComponent::Type sceneType);

private:
	struct Step
	{
		char x, y, z;
	};
	static const struct Dir
	{
		static inline const Step down{ 0, -1, 0 };
		static inline const Step north{ 0, 0, -1 };
		static inline const Step east{ 1, 0, 0 };
		static inline const Step south{ 0, 0, 1 };
		static inline const Step west{ -1, 0, 0 };
		static inline const Step up{ 0, 1, 0 };
		static inline const Step start{ 0, 0, 0 };
	};

	static Pathfinder s_instance;

	Pathfinder() noexcept;
	virtual ~Pathfinder() = default;
	DELETE_COPY_MOVE_CONSTRUCTOR(Pathfinder);

	// Methods
	
	//NavMeshID NewMesh(Box extents);
	//PortalID NewPortal(NavMeshID mesh, Box extents);
	//void ConnectMeshAndNode(NavMeshID mesh, PortalID node);
	//std::vector<Box> ConnectToNeighborsAndReturnOpen(NavMeshID mesh, Box border);
	//void GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char symbol, PortalID currentNode = MAX_ID);
	//// newPos Walk(currentPos, goal, speed)
	//// newPos Fly(currentPos, goal, speed)
	//size_t FindNavMeshContaining(const Vector3 pos);
	std::vector<PortalID> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	float heuristicStraightLine(Vector3 start, Vector3 goal);
};
