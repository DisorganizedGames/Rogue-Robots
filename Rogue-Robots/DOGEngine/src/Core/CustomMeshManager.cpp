#include "CustomMeshManager.h"
#include "../Graphics/Rendering/Renderer.h"
#include "../Graphics/Rendering/MeshTable.h"
#include "../Graphics/Rendering/UploadContext.h"

namespace DOG
{
	CustomMeshManager* CustomMeshManager::s_instance = nullptr;

	void CustomMeshManager::Initialize(gfx::Renderer* renderer)
	{
		if (!s_instance)
			s_instance = new CustomMeshManager(renderer);
	}

	void CustomMeshManager::Destroy()
	{
		if (s_instance)
		{
			delete s_instance;
			s_instance = nullptr;
		}
	}

	CustomMeshManager& CustomMeshManager::Get()
	{
		assert(s_instance);
		return *s_instance;
	}

	CustomMeshManager::CustomMeshManager(gfx::Renderer* renderer) :
		m_meshTable(renderer->GetMeshTable()),
		m_upCtx(renderer->GetMeshUploadContext())
	{

	}

	std::pair<Mesh, u32> CustomMeshManager::AddMesh(const MeshDesc& desc)
	{
		gfx::MeshTable::MeshSpecification spec;
		spec.indices = desc.indices;
		spec.submeshData = desc.submeshData;
		spec.vertexDataPerAttribute = desc.vertexDataPerAttribute;

		auto cont = m_meshTable->LoadMesh(spec, *m_upCtx);
		m_upCtx->SubmitCopies();

		return { cont.mesh, cont.numSubmeshes };
	}

	void CustomMeshManager::RemoveMesh(Mesh handle)
	{
		m_meshTable->FreeMesh(handle);
	}


}