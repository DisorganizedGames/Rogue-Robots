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
		ZoneScopedN("updateJoints_ppp");
		using namespace DirectX;
		auto deltaTime = (f32)Time::DeltaTime();

		if (!m_bonesLoaded) {
			EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& modelaC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && modelaC.offset == 0)
					{
						m_bonesLoaded = true;
						// for imgui
						m_rigs.push_back(&model->animation);
						modelaC.clips[2].currentWeight = 0.0f;
					}
				});
			return;
		}
		deltaTime = 0.05f;
		static bool firstTime = true;
		if(m_gogogo)
		{
			EntityManager::Get().Collect<ModelComponent, AnimationComponent, TransformComponent, RealAnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& animatorC, TransformComponent& tfC, RealAnimationComponent& rAC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model)
					{
						if (firstTime)
						{
							rAC.AddAnimationClip(3, 0, 0.f, 0.f, 1.0f, 1.0f, true); // lower body i
							//rAC.AddAnimationClip(4, 1, 0.f, 0.f, 1.0f, 1.0f, true); // upper body g
							rAC.AddAnimationClip(1, 2, 0.f, 0.f, 1.0f, 1.0f, true); // full body i
							//rAC.AddAnimationClip(2, 1, 0.f, 0.f, 1.0f, 1.0f, true);
							firstTime = false;
						}
						for (u32 i = 0; i < rAC.nAddedClips; ++i)
						{
							auto& clip = rAC.clips.rbegin()[i];
							auto& anim = m_rigs[0]->animations.at(clip.animationID);
							clip.SetAnimation(anim.duration, anim.ticks);
						}
						rAC.Update(deltaTime);
						if (rAC.ActiveClipCount() == 0)
							return;

						static constexpr f32 scaleFactor = 0.01f;
						UpdateClips(animatorC, deltaTime);
						if (m_imguiResetPos)
						{
							m_imguiResetPos = false;
							tfC.SetPosition({ 0.f, 0.f, 0.f });
						}
						//UpdateAnimationComponent(model->animation.animations, animatorC, (f32)Time::DeltaTime());
						UpdateSkeleton(model->animation, animatorC);
						//UpdateSkeleton(model->animation, rAC);
						if(m_up3)
							UpdateSkeleton3(model->animation, rAC);
						auto stt = 0;
					}
				});
		}
	}

	void AnimationManager::SpawnControlWindow(bool& open)
	{
		ZoneScopedN("animImgui3");

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
				ImGui::SliderFloat("groupAWeight", &m_imguiGroupWeightA, 0.0f, 1.0f, "%.5f");
				ImGui::SliderFloat("groupBWeight", &m_imguiGroupWeightB, 0.0f, 1.0f, "%.5f");

				if (ImGui::Button("gogogo")) // tmp
					m_gogogo = !m_gogogo;
				if (ImGui::Button("mu3")) // tmp
					m_up3 = !m_up3;
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				static AnimationComponent* imguiAnimC;
				static RealAnimationComponent* imguiRAC;
				static AnimationComponent::AnimationClip clip0;
				static AnimationComponent::AnimationClip clip1;
				static AnimationComponent::AnimationClip clip2;
				static i32 animation[MAX_ANIMATIONS] = { 0 };
				static i32 blendMode[MAX_ANIMATIONS] = { 0 };
				static const char* imguiBlendModes[] = { "normal", "linear", "bezier" };
				static i32 rig = 0;
				static bool rigLoaded = m_rigs.size();
				EntityManager::Get().Collect<ModelComponent, AnimationComponent, RealAnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& animatorC, RealAnimationComponent& rAC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && animatorC.offset == 0)
					{
						imguiAnimC = &animatorC;
						imguiRAC = &rAC;
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
				if (!m_up3)
				{
					ImGui::Checkbox("Apply Root translation", &m_imguiRootTranslation);

					// Set Animation Clips
					static auto setClipAnim = [&setName, animations = m_rigs[rig]->animations](i32& currentAnimIdx, u8 aIdx, AnimationComponent::AnimationClip& clip)
					{
						ImGui::Text(("Animation Clip" + std::to_string(aIdx) + " Struct\n{").c_str());
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
					static auto setClip = [&setName](u8 aIdx, AnimationComponent::AnimationClip& clip)
					{
						ImGui::SliderFloat(setName("	timeScale       ").c_str(), &clip.timeScale, m_imguiTimeScaleMin, m_imguiTimeScaleMax, "%.5f");
						ImGui::SliderFloat(setName("	currentWeight   ").c_str(), &clip.currentWeight, 0.0f, 1.0f, "%.5f");
						ImGui::SliderFloat(setName("	targetWeight    ").c_str(), &clip.targetWeight, 0.0f, 1.0f, "%.5f");
						ImGui::SliderFloat(setName("	transitionStart ").c_str(), &clip.transitionStart, 0.0f, 1.0f, "%.5f");
						ImGui::SliderFloat(setName("	transitionLength").c_str(), &clip.transitionTime, 0.0f, 1.0f, "%.5f");
						ImGui::Combo(setName("    BlendMode    ").c_str(), &blendMode[aIdx], imguiBlendModes, IM_ARRAYSIZE(imguiBlendModes));
						if (blendMode[aIdx] == 0) clip.blendMode = BlendMode::normal;
						else clip.blendMode = blendMode[aIdx] == 1 ? BlendMode::linear : BlendMode::bezier;
						ImGui::Checkbox(setName("    loop         ").c_str(), &clip.loop);
						if (ImGui::Button(("Apply Animation Clip" + std::to_string(aIdx)).c_str()))
							imguiAnimC->clips[aIdx] = clip0;
						ImGui::Text("}\n");
					};
					ImGui::Columns(MAX_ANIMATIONS, nullptr, true);
					// First Clip
					setClipAnim(animation[0], 0, clip0);
					setClip(0, clip0);
					ImGui::SliderFloat(setName("n Time").c_str(), &imguiAnimC->clips[0].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					ImGui::SliderFloat(setName("weight").c_str(), &imguiAnimC->clips[0].currentWeight, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");

					ImGui::NextColumn();
					// Second Clip
					setClipAnim(animation[1], 1, clip1);
					setClip(1, clip1);
					ImGui::SliderFloat(setName("n Time").c_str(), &imguiAnimC->clips[1].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					ImGui::SliderFloat(setName("weight").c_str(), &imguiAnimC->clips[1].currentWeight, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");

					ImGui::NextColumn();
					// Second Clip
					setClipAnim(animation[2], 2, clip2);
					setClip(2, clip2);
					ImGui::SliderFloat(setName("n Time").c_str(), &imguiAnimC->clips[2].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					ImGui::SliderFloat(setName("weight").c_str(), &imguiAnimC->clips[2].currentWeight, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");

					ImGui::Columns();
					if (ImGui::Button("Apply Clips")) // tmp
					{
						m_imguiPause = !m_imguiPause;
						imguiAnimC->clips[0] = clip0;
						imguiAnimC->clips[1] = clip1;
						imguiAnimC->clips[2] = clip2;
					}
					ImGui::SameLine(); ImGui::Checkbox("matchNormTime", &m_imguiMatching);
				}
				else
				{
					static f32 playbackRate = 1.0f;
					static i32 transitionDiv = 6;
					ImGui::SliderInt("tDiv", &transitionDiv, 2, 10, "%.5f");
					ImGui::SliderFloat("playback rate", &playbackRate, 0.01f, 2.f, "%.5f");
					if (ImGui::Button("Grenade"))
					{
						imguiRAC->AddAnimationClip(4, 1, 0.f, 0.4f, 0.f, 1.0f);
						imguiRAC->AddBlendSpecification(0.0f, .4f, 1, 1.f, 3.2f);
					}
					if (ImGui::Button("Reload"))
					{
						imguiRAC->AddAnimationClip(5, 1, 0.f, 0.4f, 0.f, 1.0f);
						imguiRAC->AddBlendSpecification(0.0f, .4f, 1, 1.f, 3.3f);
					}
					if (ImGui::Button("Shoot"))
					{
						static constexpr f32 animDuration = 0.3f;
						f32 duration = animDuration / playbackRate;
						f32 tl = duration / (f32)transitionDiv;
						imguiRAC->AddAnimationClip(6, 1, 0.f, tl, 0.f, 1.0f, false, playbackRate);
						imguiRAC->AddBlendSpecification(0.0f, tl, 1, 1.f, duration);
					}
				}

				// ImGui individual joint sliders
				if (ImGui::BeginCombo("tfs", m_rigs[0]->nodes[m_imguiSelectedBone].name.c_str()))
				{
					for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
						if (ImGui::Selectable((m_rigs[0]->nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguiSelectedBone)))
							m_imguiSelectedBone = i;
					ImGui::EndCombo();
				}
				ImGui::Text("Mask");
				ImGui::SliderInt(setName("'beginMask'").c_str(), &m_imguiMaskSpan1Strt, m_rootBoneIdx, 100);
				ImGui::SliderInt(setName("'endMask'").c_str(), &m_imguiMaskSpan1Stop, m_rootBoneIdx, 100);
				ImGui::SliderInt(setName("'beginMask2'").c_str(), &m_imguiMaskSpan2Strt, m_rootBoneIdx, 100);
				ImGui::SliderInt(setName("'endMask2'").c_str(), &m_imguiMaskSpan2Stop, m_rootBoneIdx, 100);

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
			ImGui::End(); // "Clip Setter"
		}
	}

	DirectX::FXMMATRIX AnimationManager::ImguiTransform(i32 i)
	{
		return DirectX::XMMatrixScaling(m_imguiSca[i].x, m_imguiSca[i].y, m_imguiSca[i].z) *
			DirectX::XMMatrixRotationRollPitchYaw(m_imguiRot[i].x, m_imguiRot[i].y, m_imguiRot[i].z) *
			DirectX::XMMatrixTranslation(m_imguiPos[i].x, m_imguiPos[i].y, m_imguiPos[i].z);
	}


	void AnimationManager::UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::AnimationComponent& animator)
	{
		using namespace DirectX;
		ZoneScopedN("skeletonUpdate");
		for (i32 k = 0; k < m_imguiProfilePerformUpdate; k++)
		{
			// Set node animation transformations
			std::vector<DirectX::XMMATRIX> hereditaryTFs;
			hereditaryTFs.reserve(rig.nodes.size());
			hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
			for (i32 i = 1; i < rig.nodes.size(); ++i)
			{
				auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

				if (i <= 3 || (i > m_imguiMaskSpan1Strt && i < m_imguiMaskSpan1Stop) || (i > m_imguiMaskSpan2Strt && i < m_imguiMaskSpan2Stop))
				{
					const auto sca = ExtractScaling(i, rig, animator);
					const auto rot = ExtractWeightedAvgRotation(i, rig, animator);
					const auto tra = i > m_rootBoneIdx || m_imguiRootTranslation
						? ExtractRootTranslation(i, rig, animator) : XMVECTOR{};

					if (i == 5)
					{
						m_oldDebugVec5[0] = sca;
						m_oldDebugVec5[1] = rot;
						m_oldDebugVec5[2] = tra;
					}
					ntf = XMMatrixTranspose(XMMatrixScalingFromVector(sca) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(tra));
				}

#if defined _DEBUG
				ntf *= ImguiTransform(i);
#endif
				hereditaryTFs.push_back(ntf);
			}
			if (oldFirstTime)
			{
				m_debugOldTF = hereditaryTFs;
				oldFirstTime = false;
			}
			// Apply parent Transformation
			for (size_t i = 1; i < hereditaryTFs.size(); ++i)
				hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

			const auto rootTF = XMLoadFloat4x4(&rig.nodes[0].transformation);
			for (size_t n = 0; n < rig.nodes.size(); ++n)
			{
				auto joint = rig.nodes[n].jointIdx;
				if (joint != -1)
				{
					XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
						rootTF * hereditaryTFs[n] * XMLoadFloat4x4(&rig.jointOffsets[joint]));
				}
			}
		}
	}

	void AnimationManager::UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::RealAnimationComponent& animator)
	{
		using namespace DirectX;
		ZoneScopedN("skeletonUpdate2");
		for (i32 k = 0; k < m_imguiProfilePerformUpdate; k++)
		{
			// Set node animation transformations
			std::vector<XMMATRIX> hereditaryTFs;
			std::vector<XMVECTOR> scaling;
			std::vector<XMVECTOR> rotation;
			std::vector<XMVECTOR> translation;
			SetScaling(scaling, 0, animator);
			SetRotation(rotation, 0, animator);
			SetTranslation(translation, 0, animator);

			hereditaryTFs.reserve(rig.nodes.size());
			hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
			for (i32 i = 1; i < rig.nodes.size(); ++i)
			{
				auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);
				if (i == 5)
				{
					m_newDebugVec5[0] = scaling[i];
					m_newDebugVec5[1] = rotation[i];
					m_newDebugVec5[2] = translation[i];
				}
				ntf = XMMatrixTranspose(
					XMMatrixScalingFromVector(scaling[i]) *
					XMMatrixRotationQuaternion(rotation[i]) *
					XMMatrixTranslationFromVector(translation[i])
				);

#if defined _DEBUG
				ntf *= ImguiTransform(i);
#endif
				hereditaryTFs.push_back(ntf);
			}
			if (newFirstTime)
			{
				m_debugNewTF = hereditaryTFs;
				hereditaryTFs[1] = m_debugOldTF[1];
				hereditaryTFs[2] = m_debugOldTF[2];
				auto asd = m_newDebugVec5[0];
				auto qwe = m_oldDebugVec5[0];
				newFirstTime = false;
			}
			// Apply parent Transformation
			for (size_t i = 1; i < hereditaryTFs.size(); ++i)
				hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

			const auto rootTF = XMLoadFloat4x4(&rig.nodes[0].transformation);
			for (size_t n = 0; n < rig.nodes.size(); ++n)
			{
				auto joint = rig.nodes[n].jointIdx;
				if (joint != -1)
				{
					XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
						rootTF * hereditaryTFs[n] * XMLoadFloat4x4(&rig.jointOffsets[joint]));
				}
			}
		}
	}

	void AnimationManager::UpdateSkeleton3(const DOG::ImportedRig& rig, const DOG::RealAnimationComponent& animator)
	{
		using namespace DirectX;
		ZoneScopedN("skeletonUpdate3");
		for (i32 k = 0; k < m_imguiProfilePerformUpdate; k++)
		{
			CalculateSRT(rig.animations, animator, m_mixamoIdx);
			// Set node animation transformations
			std::vector<XMMATRIX> hereditaryTFs;

			hereditaryTFs.reserve(rig.nodes.size());
			hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
			for (i32 i = 1; i < rig.nodes.size(); ++i)
			{
				auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);
				if (i == 5)
				{
					m_newNewDebugVec5[0] = m_fullbodyMixamoSRT[i * 3 + 0];
					m_newNewDebugVec5[1] = m_fullbodyMixamoSRT[i * 3 + 1];
					m_newNewDebugVec5[2] = m_fullbodyMixamoSRT[i * 3 + 2];
				}
				ntf = XMMatrixTranspose(
					XMMatrixScalingFromVector(m_fullbodyMixamoSRT[i*3+0]) *
					XMMatrixRotationQuaternion(m_fullbodyMixamoSRT[i*3+1]) *
					XMMatrixTranslationFromVector(m_fullbodyMixamoSRT[i*3+2])
				);

#if defined _DEBUG
				ntf *= ImguiTransform(i);
#endif
				hereditaryTFs.push_back(ntf);
			}
			{
				if (newNewFirstTime)
					m_debugNewTF = hereditaryTFs;
				/*hereditaryTFs[1] = m_debugOldTF[1];
				hereditaryTFs[2] = m_debugOldTF[2];*/
				auto zxc = m_newNewDebugVec5[0];
				auto asd = m_newDebugVec5[0];
				auto qwe = m_oldDebugVec5[0];
				newNewFirstTime = false;
			}
			// Apply parent Transformation
			for (size_t i = 1; i < hereditaryTFs.size(); ++i)
				hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

			const auto rootTF = XMLoadFloat4x4(&rig.nodes[0].transformation);
			for (size_t n = 0; n < rig.nodes.size(); ++n)
			{
				auto joint = rig.nodes[n].jointIdx;
				if (joint != -1)
				{
					XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
						rootTF * hereditaryTFs[n] * XMLoadFloat4x4(&rig.jointOffsets[joint]));
				}
			}
		}
	}

	DirectX::FXMVECTOR AnimationManager::GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& component, f32 tick)
	{
		using namespace DirectX;

		if (keys.size() == 1)
			return XMLoadFloat4(&keys[0].value);

		// tmp, will supply last key used in the future, for now this will have to do
		i32 key2Idx = 1;
		while (key2Idx < keys.size() - 1 && keys[key2Idx].time <= tick)
			key2Idx++;
		key2Idx = std::clamp(key2Idx, 1, i32(keys.size() - 1));
		i32 key1Idx = (key2Idx == 1) ? (i32)keys.size() - 1 : key2Idx - 1;
		
		const auto& key1 = keys[key1Idx];
		const auto& key2 = keys[key2Idx];
		const auto t1 = (key2Idx == 1) ? 0.f : key1.time;
		const auto t2 = key2.time;
		const auto blendFactor = (tick - t1) / (t2 - t1);

		return (component == KeyType::Rotation) ?
			XMQuaternionSlerp(XMLoadFloat4(&key1.value), XMLoadFloat4(&key2.value), blendFactor) :
			XMVectorLerp(XMLoadFloat4(&key1.value), XMLoadFloat4(&key2.value), blendFactor);
	}

	void AnimationManager::UpdateClips(DOG::AnimationComponent& ac, const f32 dt)
	{
		ac.Update(dt);

		f32 weightSum = 0.0f;
		for (auto& c : ac.clips)
			weightSum += c.currentWeight;

		// Normalize weights
		if(weightSum > 0)
			for (auto& c : ac.clips)
				c.currentWeight /= weightSum;

		// Tmp set matching norm time, required for run/walk etc.
		if (m_imguiMatching)
			ac.clips[2].normalizedTime = ac.clips[1].normalizedTime = ac.clips[0].normalizedTime;
	}

	DirectX::FXMVECTOR AnimationManager::ExtractRootTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		// Translation Weighted Average
		XMVECTOR translationVec = XMVECTOR{};
		for (auto& c : ac.clips)
		{
			const auto& anim = rig.animations[c.animationID];
			if (anim.posKeys.find(nodeID) != anim.posKeys.end())
				translationVec += c.currentWeight * GetKeyValue(anim.posKeys.at(nodeID), KeyType::Translation, c.currentTick);
		}
		return translationVec;
	}

	DirectX::FXMVECTOR AnimationManager::ExtractScaling(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		// Scaling Weighted Average
		XMVECTOR scaleVec = XMVECTOR{};
		bool foundKey = false;
		for (auto& c : ac.clips)
		{
			const auto& anim = rig.animations[c.animationID];
			if (anim.scaKeys.find(nodeID) != anim.scaKeys.end())
			{
				scaleVec += c.currentWeight * GetKeyValue(anim.scaKeys.at(nodeID), KeyType::Scale, c.currentTick);
				foundKey = true;
			}
		}
		return foundKey ? scaleVec : XMVECTOR{1.f, 1.f, 1.f, 1.f};
	}
	
	DirectX::FXMVECTOR AnimationManager::ExtractRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		XMVECTOR rotVec = XMVECTOR{};
		const auto& clip0 = ac.clips[0];
		const auto& clip1 = ac.clips[1];
		const auto& animation0 = rig.animations[clip0.animationID];
		const auto& animation1 = rig.animations[clip1.animationID];

		const auto keyExists = [&nodeID](const std::unordered_map<i32, std::vector<AnimationKey>>& keys)
		{ return keys.find(nodeID) != keys.end(); };
		// Rotation
		if (keyExists(animation0.rotKeys))
		{
			auto rot0 = GetKeyValue(animation0.rotKeys.at(nodeID), KeyType::Rotation, clip0.currentTick);
			auto rot1 = GetKeyValue(animation1.rotKeys.at(nodeID), KeyType::Rotation, clip1.currentTick);
			rotVec = XMQuaternionNormalize(XMQuaternionSlerp(rot0, rot1, 1.0f - clip0.currentWeight));
		}
		return rotVec;
	}

	void AnimationManager::UpdateLinearGT(AnimationComponent::AnimationClip& clip, const f32 globalTime)
	{
		// Linear transition between current weight to target weight of clip on a "global" timeline
		if (globalTime > clip.transitionStart && clip.currentWeight != clip.targetWeight)
		{
			const f32 wDiff = clip.targetWeight - clip.currentWeight;
			const f32 tCurrent = globalTime - clip.transitionStart;
			clip.currentWeight += wDiff * tCurrent / clip.transitionTime;
			clip.currentWeight = std::clamp(clip.currentWeight, 0.0f, 1.0f);
			clip.transitionTime -= tCurrent;
			clip.transitionStart += tCurrent;
		}
	}

	DirectX::FXMVECTOR AnimationManager::ExtractWeightedAvgRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		// weighted avg. of rotation quaternions, works for 'similar' rotation q's
		using namespace DirectX;
		XMVECTOR rotQ = XMVECTOR{};
		const auto& clip0 = ac.clips[0];
		const auto& animation0 = rig.animations[clip0.animationID];

		const auto keyExists = [&nodeID](const std::unordered_map<i32, std::vector<AnimationKey>>& keys)
		{ return keys.find(nodeID) != keys.end(); };
		// Rotation
		if (keyExists(animation0.rotKeys))
		{
			XMVECTOR q0 = GetKeyValue(animation0.rotKeys.at(nodeID), KeyType::Rotation, ac.clips[0].currentTick);
			for (i32 i = 0; i < ac.clips.size(); i++)
			{
				const auto& c = ac.clips[i];
				const auto& anim = rig.animations[c.animationID];
				XMVECTOR q = GetKeyValue(anim.rotKeys.at(nodeID), KeyType::Rotation, c.currentTick);
				f32 w = c.currentWeight;
				auto dot = XMVector4Dot(q0, q);
				if (i > 0 && dot.m128_f32[0] < 0.0)
					w = -w;

				rotQ += w * q;
			}
		}
		return XMVector4Normalize(rotQ);
	}
	void AnimationManager::SetGroupScalingInfluence(std::vector<DirectX::XMVECTOR>& scaling, const u8 group, const ImportedRig& rig, const RigSpecifics& rigSpec, const RealAnimationComponent& ac)
	{
		using namespace DirectX;

		const auto startNode = group == ac.groupA ? rigSpec.groupAmaskStrt : rigSpec.groupBmaskStrt;
		const auto stopNode = group == ac.groupA ? rigSpec.groupAmaskStop : rigSpec.groupBmaskStop;
		const auto nNodes = stopNode - startNode;
		const auto nClips = ac.clipsPerGroup[group];

		// Vector to store clip influences in, node adjacent
		std::vector<XMVECTOR> groupScale(nNodes * ac.clipsPerGroup[group], XMVECTOR{});

		auto startClip = 0;
		for (u8 i = 0; i < group; i++)
			startClip += ac.clipsPerGroup[i];

		// Go through all animation clips in group
		for (u8 i = 0; i < nClips; i++)
		{
			const auto clipIdx = startClip + i;
			const auto& clipData = ac.clipData[clipIdx];
			const auto& animation = rig.animations[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 node = 0; node < nNodes; node++)
			{
				const auto storeIdx = node * nClips;
				const auto rigNodeIdx = startNode + node;
				// Keyframe influence exist, store it
				if (animation.scaKeys.find(rigNodeIdx) != animation.scaKeys.end())
				{
					groupScale[storeIdx] = clipData.weight * GetKeyValue(animation.scaKeys.at(rigNodeIdx), KeyType::Scale, clipData.tick);
				}
			}
		}
		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			const auto rigNodeIdx = startNode + nodeIdx;
			for (u8 i = 0; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				scaling[rigNodeIdx] += groupScale[clipIdx];
			}

			// if no keyframe scaling influence set base scale
			if(XMComparisonAllTrue(XMVector3EqualR(scaling[rigNodeIdx], {})))
				scaling[rigNodeIdx] = XMLoadFloat3(&m_baseScale);
		}
	}

	void AnimationManager::SetScaling(std::vector<DirectX::XMVECTOR>& finalScale, const u8 rigID, const RealAnimationComponent& ac)
	{
		using namespace DirectX;
		const auto& rig = *m_rigs[rigID];
		const auto& rigSpecifics = m_rigSpecifics[rigID];

		finalScale.assign(rigSpecifics.nNodes, XMVECTOR{0, 0, 0});

		// GroupA clips influence
		if(ac.clipsPerGroup[ac.groupA])
			SetGroupScalingInfluence(finalScale, ac.groupA, rig, rigSpecifics, ac);
		// groupB clips influence
		if (ac.clipsPerGroup[ac.groupB])
			SetGroupScalingInfluence(finalScale, ac.groupB, rig, rigSpecifics, ac);

		const auto nNodes = rigSpecifics.nNodes;
		const auto nClips = ac.clipsPerGroup[ac.groupC];

		if (nClips == 0)
			return;

		auto startClip = 0;
		for (u8 i = 0; i < ac.groupC; i++)
			startClip += ac.clipsPerGroup[i];

		std::vector<XMVECTOR> groupScale(nNodes * nClips);
		groupScale.assign(nNodes * nClips, {});
		// Go through all animation clips in Fullbody clips
		for (u8 i = 0; i < nClips; i++)
		{
			const auto clipIdx = startClip + i;
			const auto& clipData = ac.clipData[clipIdx];
			const auto& animation = rig.animations[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
			{
				const auto storeIdx = nodeIdx * nClips;
				// Keyframe influence exist, store it
				if (animation.scaKeys.find(nodeIdx) != animation.scaKeys.end())
				{
					groupScale[storeIdx] = clipData.weight * GetKeyValue(animation.scaKeys.at(nodeIdx), KeyType::Scale, clipData.tick);
				}
			}
		}
		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			groupScale[nodeIdx] = groupScale[clipsStartIdx];
			for (u8 i = 1; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				groupScale[nodeIdx] += groupScale[clipIdx];
			}

			// if no keyframe scaling influence set base scale
			f32 cmpArr[4] = { 0.f, 0.f, 0.f, 0.f };
			if (XMComparisonAllTrue(XMVector3EqualR(groupScale[nodeIdx], {})))
				groupScale[nodeIdx] = XMLoadFloat3(&m_baseScale);

			// Lerp full body animation with corresponding group
			const auto lerpFactor = nodeIdx < rigSpecifics.groupAmaskStop ?
						ac.groupWeights[ac.groupA] : ac.groupWeights[ac.groupB];
			finalScale[nodeIdx] = XMVectorLerp(finalScale[nodeIdx], groupScale[nodeIdx], lerpFactor);
		}
	}

	void AnimationManager::SetGroupRotationInfluence(std::vector<DirectX::XMVECTOR>& rotation, const u8 group, const ImportedRig& rig, const RigSpecifics& rigSpec, const RealAnimationComponent& ac)
	{
		using namespace DirectX;

		const auto startNode = group == ac.groupA ? rigSpec.groupAmaskStrt : rigSpec.groupBmaskStrt;
		const auto stopNode = group == ac.groupA ? rigSpec.groupAmaskStop : rigSpec.groupBmaskStop;
		const auto nNodes = stopNode - startNode;
		const auto nClips = ac.clipsPerGroup[group];

		// Vector to store clip influences in, node adjacent
		std::vector<XMVECTOR> groupRot(ac.clipsPerGroup[group] * nNodes, XMVECTOR{});

		// save first clip idx 
		auto startClip = 0;
		for (u8 i = 0; i < group; i++)
			startClip += ac.clipsPerGroup[i];

		const RealAnimationComponent::ClipRigData* const cData = &ac.clipData[startClip];
		// Go through all animation clips in group
		for (u8 i = 0; i < nClips; i++)
		{
			const auto& clipData = cData[i];
			const auto& animation = rig.animations[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 node = 0; node < nNodes; node++)
			{
				const auto storeIdx = node * nClips;
				const auto rigNodeIdx = startNode + node;
				// Keyframe influence exist, store quaternion
				if (animation.rotKeys.find(rigNodeIdx) != animation.rotKeys.end())
				{
					groupRot[storeIdx] = GetKeyValue(animation.rotKeys.at(rigNodeIdx), KeyType::Rotation, clipData.tick);
				}
			}
		}
		
		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			const auto rigNodeIdx = startNode + nodeIdx;
			// weighted avg. quaternion from group clips
			XMVECTOR q0 = groupRot[clipsStartIdx];
			for (u8 i = 0; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				auto w = cData[i].weight;
				XMVECTOR& q = groupRot[clipIdx];
				auto dot = XMVector4Dot(q0, q);
				if (i > 0 && dot.m128_f32[0] < 0.0)
					w = -w;

				rotation[rigNodeIdx] += w * q;
			}
			rotation[rigNodeIdx] = XMVector4Normalize(rotation[rigNodeIdx]);

			// if no keyframe rotation influence set base rotation
			if (XMComparisonAllTrue(XMVector4EqualR(rotation[rigNodeIdx], {})))
				rotation[rigNodeIdx] = XMLoadFloat4(&m_baseRotation);
		}
	}

	void AnimationManager::SetRotation(std::vector<DirectX::XMVECTOR>& finalRot, const u8 rigID, const RealAnimationComponent& ac)
	{
		using namespace DirectX;
		const auto& rig = *m_rigs[rigID];
		const auto& rigSpecifics = m_rigSpecifics[rigID];

		finalRot.assign(rigSpecifics.nNodes, XMVECTOR{ 0, 0, 0, 0 });

		// GroupA clips influence
		if (ac.clipsPerGroup[ac.groupA])
			SetGroupRotationInfluence(finalRot, ac.groupA, rig, rigSpecifics, ac);
		// groupB clips influence
		if (ac.clipsPerGroup[ac.groupB])
			SetGroupRotationInfluence(finalRot, ac.groupB, rig, rigSpecifics, ac);

		const auto nNodes = rigSpecifics.nNodes;
		const auto nClips = ac.clipsPerGroup[ac.groupC];

		if (nClips == 0)
			return;

		// Get start clip of group
		auto startClip = 0;
		for (u8 i = 0; i < ac.groupC; i++)
			startClip += ac.clipsPerGroup[i];

		std::vector<XMVECTOR> groupRot(nNodes * nClips);
		// Go through all animation clips in Fullbody clips
		const RealAnimationComponent::ClipRigData* const cData = &ac.clipData[startClip];
		// Go through all animation clips in group
		for (u8 i = 0; i < nClips; i++)
		{
			const auto& clipData = cData[i];
			const auto& animation = rig.animations[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
			{
				const auto storeIdx = nodeIdx * nClips;
				// Keyframe influence exist, store quaternion
				if (animation.rotKeys.find(nodeIdx) != animation.rotKeys.end())
				{
					groupRot[storeIdx] = GetKeyValue(animation.rotKeys.at(nodeIdx), KeyType::Rotation, clipData.tick);
				}
			}
		}

		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			// weighted avg. quaternion from group clips
			XMVECTOR q0 = groupRot[clipsStartIdx];
			for (u8 i = 0; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				auto w = cData[i].weight;
				XMVECTOR& q = groupRot[clipIdx];
				// change sign if necessary
				if (i > 0 && XMVector4Dot(q0, q).m128_f32[0] < 0.0)
					w = -w;

				groupRot[nodeIdx] += w * q;
			}
			groupRot[nodeIdx] = XMVector4Normalize(groupRot[nodeIdx]);

			// Lerp full body animation with corresponding group
			const auto slerpFactor = nodeIdx < rigSpecifics.groupAmaskStop ?
				ac.groupWeights[ac.groupA] : ac.groupWeights[ac.groupB];

			finalRot[nodeIdx] = XMQuaternionSlerp(finalRot[nodeIdx], groupRot[nodeIdx], slerpFactor);
		}
	}

	void AnimationManager::SetGroupTranslationInfluence(std::vector<DirectX::XMVECTOR>& translation, const u8 group, const ImportedRig& rig, const RigSpecifics& rigSpec, const RealAnimationComponent& ac)
	{
		using namespace DirectX;

		const auto startNode = group == ac.groupA ? rigSpec.groupAmaskStrt : rigSpec.groupBmaskStrt;
		const auto stopNode = group == ac.groupA ? rigSpec.groupAmaskStop : rigSpec.groupBmaskStop;
		const auto nNodes = stopNode - startNode;
		const auto nClips = ac.clipsPerGroup[group];

		// Vector to store clip influences in, node adjacent
		std::vector<XMVECTOR> groupTrans(ac.clipsPerGroup[group] * nNodes, XMVECTOR{});

		auto startClip = 0;
		for (u8 i = 0; i < group; i++)
			startClip += ac.clipsPerGroup[i];

		// Go through all animation clips in group
		for (u8 i = 0; i < nClips; i++)
		{
			const auto clipIdx = startClip + i;
			const auto& clipData = ac.clipData[clipIdx];
			const auto& animation = rig.animations[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 node = 0; node < nNodes; node++)
			{
				const auto storeIdx = node * nClips;
				const auto rigNodeIdx = startNode + node;
				// Keyframe influence exist, store it
				if (animation.posKeys.find(rigNodeIdx) != animation.posKeys.end())
				{
					groupTrans[storeIdx] = clipData.weight * GetKeyValue(animation.posKeys.at(rigNodeIdx), KeyType::Translation, clipData.tick);
				}
			}
		}
		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			const auto rigNodeIdx = startNode + nodeIdx;
			for (u8 i = 0; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				translation[rigNodeIdx] += groupTrans[clipIdx];
			}

			// if no keyframe scaling influence set base scale
			if (XMComparisonAllTrue(XMVector3EqualR(translation[rigNodeIdx], {})))
				translation[rigNodeIdx] = XMLoadFloat3(&m_baseTranslation);
		}
	}

	void AnimationManager::SetTranslation(std::vector<DirectX::XMVECTOR>& finalTrans, const u8 rigID, const RealAnimationComponent& ac)
	{
		using namespace DirectX;
		const auto& rig = *m_rigs[rigID];
		const auto& rigSpecifics = m_rigSpecifics[rigID];

		finalTrans.assign(rigSpecifics.nNodes, XMVECTOR{ 0, 0, 0 });

		// GroupA clips influence
		if (ac.clipsPerGroup[ac.groupA])
			SetGroupTranslationInfluence(finalTrans, ac.groupA, rig, rigSpecifics, ac);
		// groupB clips influence
		if (ac.clipsPerGroup[ac.groupB])
			SetGroupTranslationInfluence(finalTrans, ac.groupB, rig, rigSpecifics, ac);

		const auto nNodes = rigSpecifics.nNodes;
		const auto nClips = ac.clipsPerGroup[ac.groupC];

		if (nClips == 0)
			return;

		// get start clipIdx of group
		auto startClip = 0;
		for (u8 i = 0; i < ac.groupC; i++)
			startClip += ac.clipsPerGroup[i];

		std::vector<XMVECTOR> groupTrans(nNodes * nClips);
		// Go through all animation clips in Fullbody clips
		for (u8 i = 0; i < nClips; i++)
		{
			const auto clipIdx = startClip + i;
			const auto& clipData = ac.clipData[clipIdx];
			const auto& animation = rig.animations[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
			{
				const auto storeIdx = nodeIdx * nClips;
				// Keyframe influence exist, store it
				if (animation.posKeys.find(nodeIdx) != animation.posKeys.end())
				{
					groupTrans[storeIdx] = clipData.weight * GetKeyValue(animation.posKeys.at(nodeIdx), KeyType::Translation, clipData.tick);
				}
			}
		}
		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			groupTrans[nodeIdx] = groupTrans[clipsStartIdx];
			for (u8 i = 1; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				groupTrans[nodeIdx] += groupTrans[clipIdx];
			}

			// if no keyframe scaling influence set base scale
			if (XMComparisonAllTrue(XMVector3EqualR(groupTrans[nodeIdx], {})))
				groupTrans[nodeIdx] = XMLoadFloat3(&m_baseScale);

			// Lerp full body animation with corresponding group
			const auto lerpFactor = nodeIdx < rigSpecifics.groupAmaskStop ?
				ac.groupWeights[ac.groupA] : ac.groupWeights[ac.groupB];

			finalTrans[nodeIdx] = XMVectorLerp(finalTrans[nodeIdx], groupTrans[nodeIdx], lerpFactor);
		}
	}

	void AnimationManager::CalculateSRT(const std::vector<AnimationData>& animations, const RealAnimationComponent& ac, const u8 rigID)
	{
		ZoneScopedN("Calc");
		using namespace DirectX;
		const auto rigSpec = m_rigSpecifics[rigID];
		const auto nSRT = 3 * rigSpec.nNodes;
		// Store scaling, translation, rotation extracted from anim keyframes
		std::fill(m_partialMixamoSRT.begin(), m_partialMixamoSRT.begin() + nSRT, XMVECTOR{});
		std::fill(m_fullbodyMixamoSRT.begin(), m_fullbodyMixamoSRT.begin() + nSRT, XMVECTOR{});

		for (u8 group = 0; group < ac.nGroups; group++)
		{
			if (ac.clipsPerGroup[group])
			{
				const auto gClipIdx = ac.GetGroupIndex(group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Scale, ac.clipsPerGroup[group], rigID, group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Rotation, ac.clipsPerGroup[group], rigID, group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Translation, ac.clipsPerGroup[group], rigID, group);
			}
		}
		// Blend between the partial body group and the full body animation group
		//const auto weightGroupA = ac.groupWeights[ac.groupA];
		const auto weightGroupB = ac.groupWeights[ac.groupB];
		const auto weightGroupA = m_imguiGroupWeightA;
		//const auto weightGroupB = m_imguiGroupWeightB;
		for (u32 i = 0; i < rigSpec.nNodes; ++i)
		{
			f32 weight = 0.0f;
			if (i < m_extra || (i >= rigSpec.groupAmaskStrt && i < rigSpec.groupAmaskStop)) // lower body
				weight = weightGroupA;
			else if (i >= rigSpec.groupBmaskStrt && i < rigSpec.groupBmaskStop) // upper body
				weight = weightGroupB;

			const u32 sIdx = i * 3, rIdx = i * 3 + 1, tIdx = i * 3 + 2;
			const XMVECTOR scaling1 = m_fullbodyMixamoSRT[sIdx], scaling2 = m_partialMixamoSRT[sIdx];
			const XMVECTOR rotation1 = m_fullbodyMixamoSRT[rIdx], rotation2 = m_partialMixamoSRT[rIdx];
			const XMVECTOR translation1 = m_fullbodyMixamoSRT[tIdx], translation2 = m_partialMixamoSRT[tIdx];
			m_fullbodyMixamoSRT[sIdx] = XMVectorLerp(scaling1, scaling2, weight);
			m_fullbodyMixamoSRT[rIdx] = XMQuaternionSlerp(rotation1, rotation2, weight);
			m_fullbodyMixamoSRT[tIdx] = XMVectorLerp(translation1, translation2, weight);
		}
		auto c = 0;
	}

	void AnimationManager::ExtractClipNodeInfluences(const ClipData* cData, const std::vector<AnimationData>& anims, const KeyType key, const u8 nClips, const u8 rigID, const u8 group)
	{
		ZoneScopedN("Extract");
		using namespace DirectX;
		using AnimationKeys = std::unordered_map<i32, std::vector<AnimationKey>>;

		const auto [startNode, nNodes] = GetNodeStartAndCount(m_rigSpecifics[rigID], group);
		const bool fullbodyGroup = group == m_rigSpecifics[rigID].fullbodyGroup;

		std::vector<XMVECTOR> keyValues(nClips * nNodes, XMVECTOR{});

		// For every clip in clip group
		for (u8 i = 0; i < nClips; i++)
		{
			const auto aID = cData[i].aID;
			const auto weight = cData[i].weight;
			const auto tick = cData[i].tick;

			const auto& animation = anims[aID];
			// Store influence that the clip animation has on each node
			for (u32 node = 0; node < nNodes; node++)
			{
				const auto storeIdx = node * nClips;
				auto rigNodeIdx = startNode + node;
				if (group == 0)
				{
					rigNodeIdx = node < m_extra ? node : rigNodeIdx-m_extra;
					auto stio = 0;
				}
				const AnimationKeys* keys = {};
				switch (key)
				{
				case KeyType::Scale:
					keys = &animation.scaKeys;
					break;
				case KeyType::Rotation:
					keys = &animation.rotKeys;
					break;
				default:
					keys = &animation.posKeys;
					break;
				}
				// Keyframe influence exist, store it
				if (keys->find(rigNodeIdx) != keys->end())
				{
					auto keyVal = GetKeyValue(keys->at(rigNodeIdx), key, tick);
					keyValues[storeIdx] = key == KeyType::Rotation ? weight * keyVal : keyVal;
				}
			}
		}
		SumNodeInfluences(keyValues, cData, key, nClips, startNode, nNodes, fullbodyGroup);
	}
	
	void AnimationManager::SumNodeInfluences(std::vector<DirectX::XMVECTOR>& keyValues, const ClipData* cData, const KeyType key, const u8 nClips, const u8 startNode, const u8 nNodes, bool fullbody)
	{
		ZoneScopedN("sumNodeINf");
		using namespace DirectX;
		static constexpr u8 nKeyValues = 3; // scale, rot, translation
		
		auto& storeSRT = fullbody ? m_fullbodyMixamoSRT : m_partialMixamoSRT;

		// Sum each influence from each clip
		for (u8 i = 0; i < nNodes; i++)
		{
			const auto frstClipIdx = i * nClips;
			auto rigNodeIdx = startNode + i;
			if (nNodes == 10 + m_extra)
			{
				rigNodeIdx = i < m_extra ? i : rigNodeIdx - m_extra;
				auto stio = 0;
			}
			const auto rigKeyIdx = nKeyValues * rigNodeIdx + (u8)key;
			// Sum clip key values for weighted average
			if(key != KeyType::Rotation)
			{
				for (u8 j = 0; j < nClips; ++j)
				{
					const auto clipIdx = frstClipIdx + j;
					storeSRT[rigKeyIdx] += keyValues[clipIdx];
				}
				// if no keyframe scaling/translation influence set base value
				if (XMComparisonAllTrue(XMVector3EqualR(storeSRT[rigKeyIdx], {})) || (rigNodeIdx == m_rootBoneIdx && key == KeyType::Translation))
				{
					const auto defaultValue = key == KeyType::Scale ? XMLoadFloat3(&m_baseScale) : XMLoadFloat3(&m_baseTranslation);
					storeSRT[rigKeyIdx] = defaultValue;
				}
			}
			else
			{
				// weighted avg. is different for rotationQuaternions
				XMVECTOR q0 = keyValues[frstClipIdx];
				for (u8 j = 0; j < nClips; ++j)
				{
					const auto clipIdx = frstClipIdx + j;
					auto w = cData[j].weight;
					XMVECTOR& q = keyValues[clipIdx];
					auto dot = XMVector4Dot(q0, q);
					if (j > 0 && dot.m128_f32[0] < 0.0)
						w = -w;

					storeSRT[rigKeyIdx] += w * q;
				}
				storeSRT[rigKeyIdx] = XMVector4Normalize(storeSRT[rigKeyIdx]);

				// if no keyframe rotation influence set base rotation
				if (XMComparisonAllTrue(XMVector4EqualR(storeSRT[rigKeyIdx], {})))
					storeSRT[rigKeyIdx] = XMLoadFloat4(&m_baseRotation);
			}
		}
	}


	void AnimationManager::SetGroupInfluence(std::vector<DirectX::XMVECTOR>& srtVector, const u8 group, const std::vector<AnimationData>& anims, const RigSpecifics& rigSpec, const RealAnimationComponent& ac)
	{
		using namespace DirectX;

		const auto startNode = group == ac.groupA ? rigSpec.groupAmaskStrt : rigSpec.groupBmaskStrt;
		const auto stopNode = group == ac.groupA ? rigSpec.groupAmaskStop : rigSpec.groupBmaskStop;
		const auto nNodes = stopNode - startNode;
		const auto nClips = ac.clipsPerGroup[group];

		// Vector to store clip influences in, node adjacent
		std::vector<XMVECTOR> groupScale(nNodes * ac.clipsPerGroup[group], XMVECTOR{});

		auto startClip = 0;
		for (u8 i = 0; i < group; i++)
			startClip += ac.clipsPerGroup[i];

		// Go through all animation clips in group
		for (u8 i = 0; i < nClips; i++)
		{
			const auto clipIdx = startClip + i;
			const auto& clipData = ac.clipData[clipIdx];
			const auto& animation = anims[clipData.aID];
			// Store influence that the clip has on each node
			for (u8 node = 0; node < nNodes; node++)
			{
				const auto storeIdx = node * nClips;
				const auto rigNodeIdx = startNode + node;
				// Keyframe influence exist, store it
				if (animation.scaKeys.find(rigNodeIdx) != animation.scaKeys.end())
				{
					groupScale[storeIdx] = clipData.weight * GetKeyValue(animation.scaKeys.at(rigNodeIdx), KeyType::Scale, clipData.tick);
				}
			}
		}
		// Sum nodewise influence from each clip
		for (u8 nodeIdx = 0; nodeIdx < nNodes; nodeIdx++)
		{
			const auto clipsStartIdx = nodeIdx * nClips;
			const auto rigNodeIdx = startNode + nodeIdx;
			for (u8 i = 0; i < nClips; i++)
			{
				const auto clipIdx = clipsStartIdx + i;
				srtVector[rigNodeIdx] += groupScale[clipIdx];
			}

			// if no keyframe scaling influence set base scale
			if (XMComparisonAllTrue(XMVector3EqualR(srtVector[rigNodeIdx], {})))
				srtVector[rigNodeIdx] = XMLoadFloat3(&m_baseScale);
		}
	}
}

