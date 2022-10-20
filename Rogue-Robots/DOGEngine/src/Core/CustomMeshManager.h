#pragma once
#include "../ECS/Component.h"

namespace DOG
{
	namespace gfx
	{
		class Renderer;
		class MeshTable;
		class UploadContext;
	}

	class CustomMeshManager
	{
	public:
		static void Initialize(gfx::Renderer* renderer);
		static void Destroy();
		static CustomMeshManager& Get();

		std::pair<Mesh, u32> AddMesh(const MeshDesc& desc);
		void RemoveMesh(Mesh handle);

	private:
		CustomMeshManager(gfx::Renderer* renderer);
		~CustomMeshManager() = default;

	private:
		static CustomMeshManager* s_instance;
		gfx::MeshTable* m_meshTable;
		gfx::UploadContext* m_upCtx{ nullptr };


	};
}