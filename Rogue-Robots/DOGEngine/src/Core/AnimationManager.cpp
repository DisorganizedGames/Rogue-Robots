#include "AnimationManager.h"
#include "ManagedAssets.h"
#include "AssetManager.h"
#include "ImGUI/imgui.h"
#include "Time.h"
#include "ImGuiMenuLayer.h"
#include "Tracy/Tracy.hpp"


namespace DOG
{
	AnimationManager::AnimationManager()
	{
		m_imguiSca.assign(150, { 1.0f, 1.0f, 1.0f });
		m_imguiPos.assign(150, { 0.0f, 0.0f, 0.0f });
		m_imguiRot.assign(150, { 0.0f, 0.0f, 0.0f });
		m_vsJoints.assign(130, {});
		ImGuiMenuLayer::RegisterDebugWindow("Animation Clip Setter", [this](bool& open) {SpawnControlWindow(open); });
	};

	AnimationManager::~AnimationManager()
	{
		ImGuiMenuLayer::UnRegisterDebugWindow("Animation Clip Setter");
	};

	void AnimationManager::UpdateJoints()
	{
		if (!m_bonesLoaded) {
			EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& modelaC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && modelaC.offset == 0)
					{
						m_bonesLoaded = true;
						// for imgui
						m_rigs.push_back(&model->animation);
					}
				});
			return;
		}
		EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& animatorC)
		{
			ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
			if (model)
			{
				UpdateAnimationComponent(model->animation.animations, animatorC, (f32)Time::DeltaTime());
				for (i32 i = 0; i < m_imguiProfilePerformUpdate; i++)
					UpdateSkeleton(model->animation, animatorC);

			}
		});
	}

	void AnimationManager::SpawnControlWindow(bool& open)
	{
		ZoneScopedN("animImgui");

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Animation Clip Setter"))
				open = true;
			ImGui::EndMenu(); // "View"
		}

		if (open)
		{
			ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Animation Clip Setter", &open))
			{
				static AnimationComponent* imguiAnimC;
				static AnimationComponent::BlendSpecification blendSpec;
				static AnimationComponent::AnimationClip clip0;
				static AnimationComponent::AnimationClip clip1;
				static i32 animation[MAX_ANIMATIONS] = { 0, 0 };
				static i32 rig = 0;
				static bool rigLoaded = m_rigs.size();
				EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& animatorC)
					{
						ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
						if (model)
						{
							imguiAnimC = &animatorC;
							if (!rigLoaded)
								m_rigs.push_back(&model->animation);
						}
						else
							return ImGui::End(); // "Animator";
					});
				if (ImGui::BeginCombo("rig", std::to_string(rig).c_str()))
				{
					for (i32 i = 0; i < std::size(m_rigs); i++)
						if (ImGui::Selectable(std::to_string(i).c_str(), (i == rig)))
							rig = i;
					ImGui::EndCombo();
				}
				i32 uniqueCount = 0;
				auto setName = [&uniqueCount](std::string&& imguiName) {
					++uniqueCount;
					std::string uniqueName = "";
					for (size_t i = 0; i < uniqueCount; i++)
						uniqueName += " ";
					ImGui::Text(imguiName.c_str());
					ImGui::SameLine();
					return uniqueName;
				};
				ImGui::Checkbox("Apply Root translation", &m_imguiRootTranslation);
				
				// Set Blend Specification
				{
					ImGui::Text("Blend Specification Struct\n{");
					ImGui::SliderFloat(setName("   blendFactor").c_str(), &blendSpec.blendFactor, m_imguiBlendMin, m_imguiBlendMax, "%.5f");
					ImGui::SliderFloat(setName("   transition ").c_str(), &blendSpec.transition, m_imguiTransitionMin, m_imguiTransitionMax, "%.5f");
					static i32 imguiMode = 0;
					static const char* imguiBlendMode[]{ "sliderBlend", "linearBlend", "bezierBlend" };
					ImGui::Combo(setName("   blendMode  ").c_str(), &imguiMode, imguiBlendMode, IM_ARRAYSIZE(imguiBlendMode));
					ImGui::Text("}\n");
					blendSpec.mode = imguiMode;
					if (ImGui::Button("Apply BlendSpec"))
						imguiAnimC->blendSpec = blendSpec;
				}

				// Set Animation Clips
				{
					auto setClipAnim = [&setName, animations = m_rigs[rig]->animations, animation = animation](u8 aIdx, AnimationComponent::AnimationClip& clip)
					{
						auto& currentAnimIdx = animation[aIdx];
						if (ImGui::BeginCombo(setName("    animation  ").c_str(), animations[currentAnimIdx].name.c_str()))
						{
							for (i32 i = 0; i < std::size(animations); i++)
								if (ImGui::Selectable(animations[i].name.c_str(), (i == currentAnimIdx)))
								{
									if (currentAnimIdx != i) // set new animation
									{
										const auto& anim = animations[i];
										clip.SetAnimation(i, anim.ticks, anim.duration);
										currentAnimIdx = i;
									}
								}
							ImGui::EndCombo();
						}
					};
					ImGui::Columns(MAX_ANIMATIONS, nullptr, true);
					// First Clip
					{
						ImGui::Text("Animation Clip0 Struct\n{");
						setClipAnim(m_animationNumeroUno, clip0);
						ImGui::SliderFloat(setName("	currentTime").c_str(), &clip0.normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
						ImGui::SliderFloat(setName("	timeScale  ").c_str(), &clip0.timeScale, m_imguiTimeScaleMin, m_imguiTimeScaleMax, "%.5f");
						ImGui::Checkbox(setName("    loop").c_str(), &clip0.loop);

						if (ImGui::Button("Apply Animation Clip0"))
							imguiAnimC->clips[0] = clip0;
						ImGui::Text("}\n");
					}
					ImGui::NextColumn();
					// Second Clip
					{
						ImGui::Text("Animation Clip1 Struct\n{");
						setClipAnim(m_animationNumeroDos, clip1);
						ImGui::SliderFloat(setName("	currentTime").c_str(), &clip1.normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
						ImGui::SliderFloat(setName("	timeScale  ").c_str(), &clip1.timeScale, m_imguiTimeScaleMin, m_imguiTimeScaleMax, "%.5f");
						ImGui::Checkbox(setName("    loop").c_str(), &clip1.loop);
						if (ImGui::Button("Apply Animation Clip1"))
							imguiAnimC->clips[1] = clip1;
						ImGui::Text("}\n");
					}
				}
				ImGui::Columns();
				if (ImGui::Button("Apply Clips & BlendSpecification"))
				{
					imguiAnimC->clips[0] = clip0;
					imguiAnimC->clips[1] = clip1;
					imguiAnimC->blendSpec = blendSpec;
				}
				ImGui::Text("				\nCurrent blendfactor\n");
				ImGui::SliderFloat(" ", &imguiAnimC->blendSpec.blendFactor, m_imguiBlendMin, m_imguiBlendMax, "%.5f");
				ImGui::Text("				\nCurrent normTime\n");
				ImGui::Columns(MAX_ANIMATIONS, nullptr, true);
				ImGui::SliderFloat(setName("	currentTime").c_str(), &imguiAnimC->clips[0].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
				ImGui::NextColumn();
				ImGui::SliderFloat(setName("	currentTime").c_str(), &imguiAnimC->clips[1].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");

				// ImGui individual joint sliders
				{
					if (ImGui::BeginCombo("tfs", m_rigs[0]->nodes[m_imguiSelectedBone].name.c_str()))
					{
						for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
							if (ImGui::Selectable((m_rigs[0]->nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguiSelectedBone)))
								m_imguiSelectedBone = i;
						ImGui::EndCombo();
					}

					ImGui::Text("Orientation");
					ImGui::SliderAngle("Roll", &m_imguiRot[m_imguiSelectedBone].z, m_imguiJointRotMin, m_imguiJointRotMax);
					ImGui::SliderAngle("Pitch", &m_imguiRot[m_imguiSelectedBone].x, m_imguiJointRotMin, m_imguiJointRotMax);
					ImGui::SliderAngle("Yaw", &m_imguiRot[m_imguiSelectedBone].y, m_imguiJointRotMin, m_imguiJointRotMax);
					ImGui::Text("Translation");
					ImGui::SliderFloat("pos X", &m_imguiPos[m_imguiSelectedBone].x, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
					ImGui::SliderFloat("pos Y", &m_imguiPos[m_imguiSelectedBone].y, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
					ImGui::SliderFloat("pos Z", &m_imguiPos[m_imguiSelectedBone].z, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
					ImGui::Text("Scale");
					ImGui::SliderFloat("X", &m_imguiSca[m_imguiSelectedBone].x, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
					ImGui::SliderFloat("Y", &m_imguiSca[m_imguiSelectedBone].y, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
					ImGui::SliderFloat("Z", &m_imguiSca[m_imguiSelectedBone].z, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
				}
			}
			ImGui::End(); // "Clip Setter"
		}
	}

	DirectX::FXMMATRIX AnimationManager::CalculateBlendTransformation(i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		XMFLOAT3 scaling = m_baseScale;
		XMFLOAT4 rotation = m_baseRotation;
		XMFLOAT3 translation = m_baseTranslation;

		const auto& clip0 = ac.clips[0];
		const auto& clip1 = ac.clips[1];
		const f32 t0 = clip0.currentTick;
		const f32 t1 = clip1.currentTick;
		const auto& animation1 = rig.animations[clip0.animationID];
		const auto& animation2 = rig.animations[clip1.animationID];

		const auto keyExists = [&nodeID](const std::unordered_map<i32, std::vector<AnimationKey>>& keys)
		{ return keys.find(nodeID) != keys.end(); };

		// Scale
		if (keyExists(animation1.scaKeys))
		{
			auto scale1 = GetAnimationComponent(animation1.scaKeys.at(nodeID), KeyType::Scale, t0);
			auto scale2 = GetAnimationComponent(animation2.scaKeys.at(nodeID), KeyType::Scale, t1);
			DirectX::XMStoreFloat3(&scaling, XMVectorLerp(scale1, scale2, ac.blendSpec.blendFactor));
		}
		// Rotation
		if (keyExists(animation1.rotKeys))
		{
			auto rot1 = GetAnimationComponent(animation1.rotKeys.at(nodeID), KeyType::Rotation, t0);
			auto rot2 = GetAnimationComponent(animation2.rotKeys.at(nodeID), KeyType::Rotation, t1);
			DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(rot1, rot2, ac.blendSpec.blendFactor));
		}
		// Translation
		const bool applyTranslation = (m_imguiRootTranslation || nodeID > m_rootBoneIdx);
		if (keyExists(animation1.posKeys) && applyTranslation)
		{
			auto translation1 = GetAnimationComponent(animation1.posKeys.at(nodeID), KeyType::Translation, t0);
			auto translation2 = GetAnimationComponent(animation2.posKeys.at(nodeID), KeyType::Translation, t1);
			DirectX::XMStoreFloat3(&translation, XMVectorLerp(translation1, translation2, ac.blendSpec.blendFactor));
		}

		return XMMatrixTranspose(XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
			XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
			XMMatrixTranslation(translation.x, translation.y, translation.z));
	}

	void AnimationManager::UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::AnimationComponent& animator)
	{
		ZoneScopedN("skeletonUpdate");

		const auto& clip0 = animator.clips[0];
		const auto& clip1 = animator.clips[1];

		// Set node animation transformations
		std::vector<DirectX::XMMATRIX> hereditaryTFs;
		hereditaryTFs.reserve(rig.nodes.size());
		hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); ++i)
		{
			auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

			if (clip0.HasActiveAnimation() && clip1.HasActiveAnimation())
				ntf = CalculateBlendTransformation(i, rig, animator);
			else if (clip0.HasActiveAnimation())
				ntf = CalculateNodeTransformation(rig.animations[clip0.animationID], i, clip0.currentTick);
			else if (clip1.HasActiveAnimation())
				ntf = CalculateNodeTransformation(rig.animations[clip1.animationID], i, clip1.currentTick);
			else
				CalculateNodeTransformation(rig.animations[0], i, clip0.currentTick);
			
#if defined _DEBUG
			DirectX::XMMATRIX imguiMatrix = DirectX::XMMatrixScaling(m_imguiSca[i].x, m_imguiSca[i].y, m_imguiSca[i].z) *
				DirectX::XMMatrixRotationRollPitchYaw(m_imguiRot[i].x, m_imguiRot[i].y, m_imguiRot[i].z) *
				DirectX::XMMatrixTranslation(m_imguiPos[i].x, m_imguiPos[i].y, m_imguiPos[i].z);

			hereditaryTFs.back() *= imguiMatrix;
#endif
			hereditaryTFs.push_back(ntf);
		}
		// Apply parent Transformation
		for (size_t i = 1; i < hereditaryTFs.size(); ++i)
			hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

		const auto rootTF = DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation);
		for (size_t n = 0; n < rig.nodes.size(); ++n)
		{
			auto joint = rig.nodes[n].jointIdx;
			if (joint != -1)
				DirectX::XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
					rootTF * hereditaryTFs[n] * DirectX::XMLoadFloat4x4(&rig.jointOffsets[joint]));
		}
	}

	DirectX::FXMMATRIX AnimationManager::CalculateNodeTransformation(const DOG::AnimationData& animation, i32 nodeID, f32 tick)
	{
		using namespace DirectX;
		XMFLOAT3 scaling = m_baseScale;
		XMFLOAT4 rotation = m_baseRotation;
		XMFLOAT3 translation = m_baseTranslation;

		const auto keyExists = [&nodeID](const std::unordered_map<i32, std::vector<AnimationKey>>& keys)
		{ return keys.find(nodeID) != keys.end(); };
		const auto thereIsOnlyOneKeyIn = [](const std::vector<AnimationKey>& keys)
		{ return keys.size() == 1; };

		// Scaling
		if (keyExists(animation.scaKeys))
		{
			const auto& scalingKeys = animation.scaKeys.at(nodeID);
			if (thereIsOnlyOneKeyIn(scalingKeys))
				scaling = { scalingKeys[0].value.x, scalingKeys[0].value.y, scalingKeys[0].value.z };
			else
				DirectX::XMStoreFloat3(&scaling, GetAnimationComponent(animation.scaKeys.at(nodeID), KeyType::Scale, tick));
		}
		// Rotation
		if (keyExists(animation.rotKeys))
		{
			const auto& rotKeys = animation.rotKeys.at(nodeID);
			if (thereIsOnlyOneKeyIn(rotKeys))
				rotation = { rotKeys[0].value.x, rotKeys[0].value.y, rotKeys[0].value.z, rotKeys[0].value.w };
			else
				DirectX::XMStoreFloat4(&rotation, GetAnimationComponent(animation.rotKeys.at(nodeID), KeyType::Rotation, tick));
		}
		// Translation
		const bool applyTranslation = (m_imguiRootTranslation || nodeID > m_rootBoneIdx);
		if (keyExists(animation.posKeys) && applyTranslation)
		{
			const auto& posKeys = animation.posKeys.at(nodeID);
			if (thereIsOnlyOneKeyIn(posKeys))
				translation = { posKeys[0].value.x, posKeys[0].value.y, posKeys[0].value.z };
			else
				DirectX::XMStoreFloat3(&translation, GetAnimationComponent(animation.posKeys.at(nodeID), KeyType::Translation, tick));
		}

		auto nodeTransform = XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
			XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
			XMMatrixTranslation(translation.x, translation.y, translation.z);

		return XMMatrixTranspose(nodeTransform);
	}

	DirectX::FXMVECTOR AnimationManager::GetAnimationComponent(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick)
	{
		using namespace DirectX;

		if (keys.size() == 1)
			return XMLoadFloat4(&keys[0].value);

		// tmp, animations will supply last key used in the future, for now this will have to do
		i32 key2Idx = 1;
		while (key2Idx < keys.size() - 1 && keys[key2Idx].time <= tick)
			key2Idx++;
		key2Idx = std::clamp(key2Idx, 1, i32(keys.size() - 1));
		i32 key1Idx = key2Idx - 1;

		if (component == KeyType::Rotation)
			return XMQuaternionSlerp(XMLoadFloat4(&keys[key1Idx].value), XMLoadFloat4(&keys[key2Idx].value),
				(tick - keys[key1Idx].time) / (keys[key2Idx].time - keys[key1Idx].time));
		else
			return XMVectorLerp(XMLoadFloat4(&keys[key1Idx].value), XMLoadFloat4(&keys[key2Idx].value),
				(tick - keys[key1Idx].time) / (keys[key2Idx].time - keys[key1Idx].time));
	}

	void AnimationManager::UpdateAnimationComponent(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt) const
	{
		// tmp code for testing different blending updates
		/*switch (ac.mode)
		{
		case m_modeImguiBlend:

			break;
		default:
			break;
		}*/
		ZoneScopedN("updateAnimComponent");
		if (ac.mode == m_modeImguiBlend) // tmp loop and blend with ImGui
		{
			for (u8 i = 0; i < MAX_ANIMATIONS; i++)
			{
				if (ac.HasActiveAnimation(i))
				{
					const auto& animation = animations[ac.animationID[i]];
					ac.normalizedTime[i] += ac.timeScale[i] * dt / animation.duration;
					while (ac.normalizedTime[i] > 1.0f)
						ac.normalizedTime[i] -= 1.0f;
					if (ac.normalizedTime[i] < 0.0f)
						ac.normalizedTime[i] = 1.0f + ac.normalizedTime[i];

					ac.tick[i] = ac.normalizedTime[i] * animation.ticks;
				}
			}
		}
		else if (ac.blendSpec.mode == m_modeTransitionBezierBlend) // tmp Transititon test
		{
			clip0.UpdateClip(dt);
			if (clip0.normalizedTime > ac.blendSpec.transition)
			{
				clip1.UpdateClip(dt);
				f32 tstart = ac.blendSpec.transition;
				f32 t = clip0.normalizedTime - tstart;
				f32 u = t / (1.0f - tstart);
				f32 v = (1 - u);
				f32 bt = 3.f * v * u * u + std::powf(u, 3.f);
				ac.blendSpec.blendFactor = bt;
			}
			ac.blendSpec.blendFactor = std::clamp(ac.blendSpec.blendFactor, 0.0f, 1.0f);

			if (clip0.normalizedTime > m_imguiNormalizedTimeMax)
			{
				ac.blendSpec.blendFactor = 1.0f;
				clip0.normalizedTime = m_imguiNormalizedTimeMin;
				clip0.animationID = m_noAnimation;
				ac.blendSpec.mode = m_modeImguiBlend;
				return;
			}
		}
	}

	void AnimationManager::UpdateMovementAnimation(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt)
	{
		auto& clip0 = ac.clips[0];
		auto& clip1 = ac.clips[1];
		
		// TEST idle->walk
		clip0.animationID = 2;
		clip1.animationID = 3;

		// Frozen transition
		clip0.timeScale = 0.0f;
	}
}

