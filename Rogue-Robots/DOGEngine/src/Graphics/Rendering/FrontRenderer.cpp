#include "FrontRenderer.h"
#include "Renderer.h"
#include "../../Core/AssetManager.h"			// Asset translation
#include "../../Physics/PhysicsEngine.h"		// Collider components

// Exposed graphics managers
#include "../../Core/CustomMaterialManager.h"
#include "../../Core/CustomMeshManager.h"
#include "../../Core/LightManager.h"		

#include "../../common/MiniProfiler.h"

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

	void FrontRenderer::Update(f32 dt)
	{
		MINIPROFILE;

		UpdateLights();
		GatherShadowCasters();
		SetRenderCamera();
		GatherDrawCalls();
		CullShadowDraws();

		// Update internal data structures
		m_renderer->Update(dt);

		// Clear state
		m_singleSidedShadowed.clear();
		m_doubleSidedShadowed.clear();
	}

	void FrontRenderer::UpdateLights()
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
	}

	void FrontRenderer::GatherDrawCalls()
	{
		auto& mgr = EntityManager::Get();

		mgr.Collect<TransformComponent, SubmeshRenderer>().Do([&](entity e, TransformComponent& tr, SubmeshRenderer& sr)
			{
				// We are assuming that this is a totally normal submesh with no weird branches (i.e on ModularBlock or whatever)
				if (mgr.HasComponent<ShadowReceiverComponent>(e))
				{
					m_singleSidedShadowed.push_back({ sr.mesh, 0, tr });
				}
				if (sr.dirty)
					CustomMaterialManager::Get().UpdateMaterial(sr.material, sr.materialDesc);
				m_renderer->SubmitMesh(sr.mesh, 0, sr.material, tr);
			});

		// We need to bucket in a better way..
		mgr.Bundle<TransformComponent, ModelComponent>().Do([&](entity e, TransformComponent& transformC, ModelComponent& modelC)
			{
				TransformComponent camTransform;
				camTransform.worldMatrix = ((DirectX::SimpleMath::Matrix)m_viewMat).Invert();
				auto&& cull = [camForward = camTransform.GetForward(), camPos = camTransform.GetPosition()](DirectX::SimpleMath::Vector3 p) {
					auto d = p - camPos;
					auto lenSq = d.LengthSquared();
					if (lenSq < 64) return false;
					if (lenSq > 80 * 80) return true;
					d.Normalize();
					return camForward.Dot(d) < 0.2f;
				};

				if (cull({ transformC.worldMatrix(3, 0), transformC.worldMatrix(3, 1), transformC.worldMatrix(3, 2) }))
					return;

				ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
				if (model && model->gfxModel)
				{
					// Shadow submission:
					if (mgr.HasComponent<ShadowReceiverComponent>(e))
					{
						for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
						{
							if (mgr.HasComponent<ModularBlockComponent>(e))
								m_doubleSidedShadowed.push_back({ model->gfxModel->mesh.mesh, i, transformC, false });
							else if (EntityManager::Get().HasComponent<AnimationComponent>(e))
								m_doubleSidedShadowed.push_back({ model->gfxModel->mesh.mesh, i, transformC, false, true, EntityManager::Get().GetComponent<AnimationComponent>(e).offset });
							else
								m_singleSidedShadowed.push_back({ model->gfxModel->mesh.mesh, i, transformC });
						}
					}

					if (mgr.HasComponent<ModularBlockComponent>(e))
					{
						if (mgr.HasComponent<MeshColliderComponent>(e) &&
							mgr.GetComponent<MeshColliderComponent>(e).drawMeshColliderOverride)
						{
							u32 meshColliderModelID = mgr.GetComponent<MeshColliderComponent>(e).meshColliderModelID;
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
					else if (mgr.HasComponent<AnimationComponent>(e))
					{
						auto offset = mgr.GetComponent<AnimationComponent>(e).offset;
						if (!mgr.HasComponent<DontDraw>(e))
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitAnimatedMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC, offset);
					}
					else
					{
						if (mgr.HasComponent<MeshColliderComponent>(e) &&
							mgr.GetComponent<MeshColliderComponent>(e).drawMeshColliderOverride)
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
	
	
	}

	void FrontRenderer::SetRenderCamera()
	{
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

		m_viewMat = cameraComponent.viewMatrix;
	}

	void FrontRenderer::GatherShadowCasters()
	{
		// Collect this frames spotlight shadow casters
		m_activeSpotlightShadowCasters.clear();
		EntityManager::Get().Collect</*ShadowCasterComponent, */SpotLightComponent, CameraComponent, TransformComponent>().Do([&](
			entity spotlightEntity, /*ShadowCasterComponent&, */ SpotLightComponent & slc, CameraComponent& cc, TransformComponent& tc)
			{				
				// Register this frames spotlights
				Renderer::ActiveSpotlight spotData{};

				spotData.position = { tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z, 1.0f };
				spotData.color = { slc.color.x, slc.color.y, slc.color.z, };
				spotData.direction = slc.direction;
				spotData.cutoffAngle = slc.cutoffAngle;
				spotData.strength = slc.strength;

				if (EntityManager::Get().HasComponent<ShadowCasterComponent>(spotlightEntity))
				{
					spotData.shadow = Renderer::ShadowCaster();
					spotData.shadow->viewMat = cc.viewMatrix;
					spotData.shadow->projMat = cc.projMatrix;
					u32 shadowID = *m_renderer->RegisterSpotlight(spotData);
					m_activeSpotlightShadowCasters.push_back({ spotlightEntity, shadowID });
				}
				else
				{
					spotData.shadow = std::nullopt;
					m_renderer->RegisterSpotlight(spotData);
				}
				
				//u32 shadowID = *m_renderer->RegisterSpotlight(spotData);
				//std::optional<u32> shadowID = m_renderer->RegisterSpotlight(spotData);
				//if (shadowID != std::nullopt)
				//	m_activeSpotlightShadowCasters.push_back({ spotlightEntity, *shadowID });
			});

		// Update renderer shadow map capacity
		if (m_activeSpotlightShadowCasters.size() > m_shadowMapCapacity)
		{
			std::cout << "dirty! went from " << m_shadowMapCapacity << " to " << m_activeSpotlightShadowCasters.size() << "\n";
			m_shadowMapCapacity = (u32)m_activeSpotlightShadowCasters.size();

			auto settings = m_renderer->GetGraphicsSettings();
			settings.shadowMapCapacity = m_shadowMapCapacity;
			m_renderer->SetGraphicsSettings(settings);
		}
	}

	void FrontRenderer::CullShadowDraws()
	{
		// Cull all shadowed submissions per caster
		for (const auto& [caster, shadowID]: m_activeSpotlightShadowCasters)
		{
			auto& cc = EntityManager::Get().GetComponent<CameraComponent>(caster);
			TransformComponent camTransform;
			camTransform.worldMatrix = ((DirectX::SimpleMath::Matrix)cc.viewMatrix).Invert();
			auto&& cull = [camForward = camTransform.GetForward(), camPos = camTransform.GetPosition()](DirectX::SimpleMath::Vector3 p) {
				auto d = p - camPos;
				if (d.LengthSquared() < 64) return false;
				if (d.LengthSquared() > 80 * 80) return true;
				d.Normalize();
				return camForward.Dot(d) < 0.2f;
			};

			for (const auto& sub : m_singleSidedShadowed)
			{
				if (cull({ sub.tc.worldMatrix(3, 0), sub.tc.worldMatrix(3, 1), sub.tc.worldMatrix(3, 2) }))
					continue;
				if (sub.animated)
					m_renderer->SubmitSingleSidedShadowMesh(shadowID, sub.mesh, sub.submesh, sub.tc, true, sub.jointOffset);
				else
					m_renderer->SubmitSingleSidedShadowMesh(shadowID, sub.mesh, sub.submesh, sub.tc);
			}

			for (const auto& sub : m_doubleSidedShadowed)
			{
				if (cull({ sub.tc.worldMatrix(3, 0), sub.tc.worldMatrix(3, 1), sub.tc.worldMatrix(3, 2) }))
					continue;
				if (sub.animated)
					m_renderer->SubmitDoubleSidedShadowMesh(shadowID, sub.mesh, sub.submesh, sub.tc, true, sub.jointOffset);
				else
					m_renderer->SubmitDoubleSidedShadowMesh(shadowID, sub.mesh, sub.submesh, sub.tc);
			}
		}

	}

	void FrontRenderer::Render(f32)
	{
		m_renderer->Render(0.f);
	}

	void FrontRenderer::PerformDeferredDeletion()
	{
		LightManager::Get().DestroyDeferredEntities();

	}

	void FrontRenderer::ToggleShadowMapping(bool turnOn)
	{
		if (turnOn)
		{
			EntityManager::Get().Collect<SpotLightComponent>().Do([](entity spotlight, SpotLightComponent&)
				{
					EntityManager::Get().AddComponent<ShadowCasterComponent>(spotlight);
				});
		}
		else
		{
			EntityManager::Get().Collect<ShadowCasterComponent>().Do([](entity spotlight, ShadowCasterComponent&)
				{
					EntityManager::Get().RemoveComponent<ShadowCasterComponent>(spotlight);
				});
		}
	}
}