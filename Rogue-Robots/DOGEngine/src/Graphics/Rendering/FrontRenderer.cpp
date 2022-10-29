#include "FrontRenderer.h"
#include "Renderer.h"
#include "../../ECS/EntityManager.h"			// Grab world state
#include "../../Core/AssetManager.h"			// Asset translation
#include "../../Physics/PhysicsEngine.h"		// Collider components

// Exposed graphics managers
#include "../../Core/CustomMaterialManager.h"
#include "../../Core/CustomMeshManager.h"
#include "../../Core/LightManager.h"		

namespace DOG::gfx
{
	FrontRenderer::FrontRenderer(Renderer* renderer) :
		m_renderer(renderer)
	{
		LightManager::Initialize(m_renderer);
		CustomMeshManager::Initialize(m_renderer);
		CustomMaterialManager::Initialize(m_renderer);

	}

	FrontRenderer::~FrontRenderer()
	{
		LightManager::Destroy();
		CustomMeshManager::Destroy();
		CustomMaterialManager::Destroy();
	}

	void FrontRenderer::BeginFrameUICapture()
	{
		m_renderer->BeginGUI();
	}

	void FrontRenderer::BeginGPUFrame()
	{
		m_renderer->BeginFrame_GPU();
	}

	void FrontRenderer::EndGPUFrame()
	{
		m_renderer->EndFrame_GPU(true);
	}

	void FrontRenderer::Update(f32)
	{
		// Update lights
		EntityManager::Get().Collect<DirtyComponent, PointLightComponent>().Do([](entity, DirtyComponent& dirty, PointLightComponent& light) {
			light.dirty |= dirty.IsDirty(DirtyComponent::positionChanged); });

		EntityManager::Get().Collect<DirtyComponent, SpotLightComponent>().Do([](entity, DirtyComponent& dirty, SpotLightComponent& light) {
			light.dirty |= dirty.IsDirty(DirtyComponent::positionChanged) || dirty.IsDirty(DirtyComponent::rotationChanged); });

		EntityManager::Get().Collect<TransformComponent, SpotLightComponent>().Do([&](entity, TransformComponent tr, SpotLightComponent& light)
			{
				if (light.dirty)
				{
					SpotLightDesc d{};
					d.position = tr.GetPosition();
					d.color = light.color;
					d.cutoffAngle = light.cutoffAngle;
					d.direction = light.direction;
					d.strength = light.strength;
					d.id = light.id;
					LightManager::Get().UpdateSpotLight(light.handle, d);
					light.dirty = false;
				}
			});

		EntityManager::Get().Collect<TransformComponent, PointLightComponent>().Do([&](entity, TransformComponent tr, PointLightComponent& light)
			{
				if (light.dirty)
				{
					PointLightDesc d{};
					d.position = tr.GetPosition();
					d.color = light.color;
					d.strength = light.strength;
					LightManager::Get().UpdatePointLight(light.handle, d);
					light.dirty = false;
				}
			});


		EntityManager::Get().Collect<TransformComponent, SubmeshRenderer>().Do([&](entity e, TransformComponent& tr, SubmeshRenderer& sr)
			{
				// We are assuming that this is a totally normal submesh with no weird branches (i.e on ModularBlock or whatever)
				if (EntityManager::Get().HasComponent<ShadowReceiverComponent>(e))
				{
					m_renderer->SubmitShadowMesh(sr.mesh, 0, sr.material, tr);
				}
				if (sr.dirty)
					CustomMaterialManager::Get().UpdateMaterial(sr.material, sr.materialDesc);
				m_renderer->SubmitMesh(sr.mesh, 0, sr.material, tr);
			});

		// We need to bucket in a better way..
		EntityManager::Get().Collect<TransformComponent, ModelComponent>().Do([&](entity e, TransformComponent& transformC, ModelComponent& modelC)
			{
				MINIPROFILE_NAMED("RenderSystem")
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
				if (model && model->gfxModel)
				{
					// Shadow submission:
					if (EntityManager::Get().HasComponent<ShadowReceiverComponent>(e))
					{
						for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
						{
							if (EntityManager::Get().HasComponent<ModularBlockComponent>(e))
								m_renderer->SubmitShadowMeshNoFaceCulling(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
							else
								m_renderer->SubmitShadowMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}
					}


					if (EntityManager::Get().HasComponent<ModularBlockComponent>(e))
					{
						if (EntityManager::Get().HasComponent<MeshColliderComponent>(e) &&
							EntityManager::Get().GetComponent<MeshColliderComponent>(e).drawMeshColliderOverride)
						{
							u32 meshColliderModelID = EntityManager::Get().GetComponent<MeshColliderComponent>(e).meshColliderModelID;
							ModelAsset* meshColliderModel = AssetManager::Get().GetAsset<ModelAsset>(meshColliderModelID);
							if (meshColliderModel && meshColliderModel->gfxModel)
							{
								for (u32 i = 0; i < meshColliderModel->gfxModel->mesh.numSubmeshes; ++i)
									m_renderer->SubmitMeshWireframeNoFaceCulling(meshColliderModel->gfxModel->mesh.mesh, i, meshColliderModel->gfxModel->mats[i], transformC);
							}
						}
						else
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitMeshNoFaceCulling(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}
					}
					else if (EntityManager::Get().HasComponent<AnimationComponent>(e))
					{
						for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
							m_renderer->SubmitAnimatedMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
					}
					else
					{
						if (EntityManager::Get().HasComponent<MeshColliderComponent>(e) &&
							EntityManager::Get().GetComponent<MeshColliderComponent>(e).drawMeshColliderOverride)
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitMeshWireframe(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}
						else
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}
					}
				}
			});

		CameraComponent cameraComponent;
		EntityManager::Get().Collect<CameraComponent>().Do([&](CameraComponent& c)
			{
				if (c.isMainCamera)
				{
					cameraComponent = c;
				}
			});

		auto& proj = (DirectX::XMMATRIX&)cameraComponent.projMatrix;
		m_renderer->SetMainRenderCamera(cameraComponent.viewMatrix, &proj);

		// Finalize updates
		m_renderer->Update(0.f);
	}

	void FrontRenderer::Render(f32)
	{
		Update(0.f);

		m_renderer->Render(0.f);
	}

	void FrontRenderer::PerformDeferredDeletion()
	{
		LightManager::Get().DestroyDeferredEntities();

	}


}