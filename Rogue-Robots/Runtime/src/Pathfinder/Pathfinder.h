#pragma once
#include <DOGEngine.h>
#include "../Game/GameComponent.h"
#include "PathfinderComponents.h"

class Pathfinder
{
using Vector3 = DirectX::SimpleMath::Vector3;
private:
	static void Init();
	bool m_visualizePaths;
	bool m_vizNavMeshes;
	bool m_vizPortals;

public:
	using NavMeshID = DOG::entity;
	using PortalID = DOG::entity;

	// public methods
	[[nodiscard]] static constexpr Pathfinder& Get() noexcept
	{ 
		if (m_initialized == false)
			Init(); 
		return s_instance;
	}

	void BuildNavScene(SceneComponent::Type sceneType);

	std::vector<Vector3> Checkpoints(Vector3 start, Vector3 goal);
	void Checkpoints(Vector3 start, PathfinderWalkComponent& pfc);

	bool DrawPaths();

private:
	struct Step
	{
		char x, y, z;
	};
	const struct Dir
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
	static bool m_initialized;

	Pathfinder() noexcept;
	~Pathfinder();
	DELETE_COPY_MOVE_CONSTRUCTOR(Pathfinder);

	void VisualizePathsMenu(bool& open);

	// Methods
	std::vector<PortalID> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	static float heuristicStraightLine(Vector3 start, Vector3 goal);
};
