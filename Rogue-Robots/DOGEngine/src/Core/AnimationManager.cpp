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
		//static bool open = true;
		//SpawnControlWindow(open);
		EntityManager::Get().Collect<ModelComponent, AnimationComponent, TransformComponent>().Do([&](ModelComponent& modelC, AnimationComponent& animatorC, TransformComponent& tfC)
		{
			ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
			if (model)
			{
				if (m_imguiTestMovement)
				{
					UpdateMovementAnimation(animatorC, (f32)Time::DeltaTime());
					auto trs = CalculateTranslation(m_rootBoneIdx, model->animation, animatorC) * 0.01f;
					trs *= m_imguiMovementSpeed < 0.f ? -1.f : 1.f;
					if(m_imguiRootTranslation)
						tfC.SetPosition(XMVectorAdd(tfC.GetPosition(), trs));
				}
				else
				{
					UpdateClips(animatorC, (f32)Time::DeltaTime());
					auto trs = CalculateTranslation(m_rootBoneIdx, model->animation, animatorC) * 0.01f;
					trs *= m_imguiMovementSpeed < 0.f ? -1.f : 1.f;
					if (m_imguiRootTranslation)
						tfC.SetPosition(XMVectorAdd(tfC.GetPosition(), trs));
				}
				if (m_imguiResetPos)
				{
					m_imguiResetPos = false;
					tfC.SetPosition({ 0.f, 0.f, 0.f });
				}
				//UpdateAnimationComponent(model->animation.animations, animatorC, (f32)Time::DeltaTime());
				for (i32 i = 0; i < m_imguiProfilePerformUpdate; i++)
					UpdateSkeleton(model->animation, animatorC);
			}
		});
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
				static AnimationComponent* imguiAnimC;
				static AnimationComponent::AnimationClip clip0;
				static AnimationComponent::AnimationClip clip1;
				static AnimationComponent::AnimationClip clip2;
				static i32 animation[MAX_ANIMATIONS] = { 0 };
				static i32 blendMode[MAX_ANIMATIONS] = { 0 };
				static const char* imguiBlendModes[] = { "normal", "linear", "bezier" };
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
				ImGui::Checkbox("testMovement", &m_imguiTestMovement);
				if (ImGui::Button("reset position"))
					m_imguiResetPos = true;
				if (m_imguiTestMovement)
				{
					ImGui::SliderFloat(setName("movementSpeed").c_str(), &m_imguiMovementSpeed, -3.0f, 3.0f, "%.5f");
					ImGui::End(); // "Clip Setter"
					return;
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
					auto setClip = [&setName, &setClipAnim](u8 aIdx, AnimationComponent::AnimationClip& clip)
					{
						ImGui::Text(("Animation Clip" + std::to_string(aIdx) + " Struct\n{").c_str());
						setClipAnim(aIdx, clip);
						ImGui::SliderFloat(setName("	timeScale       ").c_str(), &clip.timeScale, m_imguiTimeScaleMin, m_imguiTimeScaleMax, "%.5f");
						ImGui::SliderFloat(setName("	currentWeight   ").c_str(), &clip.currentWeight, 0.0f, 1.0f, "%.5f");
						ImGui::SliderFloat(setName("	targetWeight    ").c_str(), &clip.targetWeight, 0.0f, 1.0f, "%.5f");
						ImGui::SliderFloat(setName("	transitionStart ").c_str(), &clip.transitionStart, 0.0f, 1.0f, "%.5f");
						ImGui::SliderFloat(setName("	transitionLength").c_str(), &clip.transitionTime, 0.0f, 1.0f, "%.5f");
						ImGui::Combo(setName("    BlendMode    ").c_str(), &blendMode[aIdx], imguiBlendModes, IM_ARRAYSIZE(imguiBlendModes));
						if (blendMode[aIdx] == 0) clip.blendMode = AnimationBlendMode::normal;
						else clip.blendMode = blendMode[aIdx] == 1 ? AnimationBlendMode::linear : AnimationBlendMode::bezier;
						ImGui::Checkbox(setName("    loop         ").c_str(), &clip.loop);
						if (ImGui::Button(("Apply Animation Clip"+std::to_string(aIdx)).c_str()))
							imguiAnimC->clips[aIdx] = clip0;
						ImGui::Text("}\n");
					};
					ImGui::Columns(MAX_ANIMATIONS, nullptr, true);
					// First Clip
					setClip(0, clip0);
					ImGui::SliderFloat(setName("n Time").c_str(), &imguiAnimC->clips[0].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					ImGui::SliderFloat(setName("weight").c_str(), &imguiAnimC->clips[0].currentWeight, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");

					ImGui::NextColumn();
					// Second Clip
					setClip(1, clip1);
					ImGui::SliderFloat(setName("n Time").c_str(), &imguiAnimC->clips[1].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					ImGui::SliderFloat(setName("weight").c_str(), &imguiAnimC->clips[1].currentWeight, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					
					ImGui::NextColumn();
					// Second Clip
					setClip(2, clip2);
					ImGui::SliderFloat(setName("n Time").c_str(), &imguiAnimC->clips[2].normalizedTime, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
					ImGui::SliderFloat(setName("weight").c_str(), &imguiAnimC->clips[2].currentWeight, m_imguiNormalizedTimeMin, m_imguiNormalizedTimeMax, "%.5f");
				}
				ImGui::Columns();
				if (ImGui::Button("Apply Clips")) // tmp
				{
					m_imguiPause = !m_imguiPause;
					imguiAnimC->clips[0] = clip0;
					imguiAnimC->clips[1] = clip1;
					imguiAnimC->clips[2] = clip2;
				}
				ImGui::SameLine(); ImGui::Checkbox("matchNormTime", &m_imguiMatching);

				// ImGui individual joint sliders
				{
					if (ImGui::BeginCombo("tfs", m_rigs[0]->nodes[m_imguiSelectedBone].name.c_str()))
					{
						for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
							if (ImGui::Selectable((m_rigs[0]->nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguiSelectedBone)))
								m_imguiSelectedBone = i;
						ImGui::EndCombo();
					}
					ImGui::Text("Mask");
					ImGui::SliderInt(setName("'minMask'").c_str(), &m_imguiMinMaskIdx, 0, 100);
					ImGui::SliderInt(setName("'maxMask'").c_str(), &m_imguiMaxMaskIdx, 0, 100);

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

		// Set node animation transformations
		std::vector<DirectX::XMMATRIX> hereditaryTFs;
		hereditaryTFs.reserve(rig.nodes.size());
		hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); ++i)
		{
			auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

			if (i < m_imguiMinMaskIdx || i > m_imguiMaxMaskIdx)
			{
				const auto sca = CalculateScaling(i, rig, animator);
				const auto rot = CalculateRotation2(i, rig, animator);
				const auto tra = i > m_rootBoneIdx ? CalculateTranslation(i, rig, animator) : XMVECTOR{};
				ntf = XMMatrixTranspose(XMMatrixScalingFromVector(sca) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(tra));
			}

#if defined _DEBUG
			ntf *= ImguiTransform(i);
#endif
			hereditaryTFs.push_back(ntf);
		}
		// Apply parent Transformation
		for (size_t i = 1; i < hereditaryTFs.size(); ++i)
			hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

		const auto rootTF = XMLoadFloat4x4(&rig.nodes[0].transformation);
		for (size_t n = 0; n < rig.nodes.size(); ++n)
		{
			auto joint = rig.nodes[n].jointIdx;
			if (joint != -1)
				XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
					rootTF * hereditaryTFs[n] * XMLoadFloat4x4(&rig.jointOffsets[joint]));
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
		f32 weightSum = 0.0f;
		for (auto& c : ac.clips)
		{
			c.UpdateClip(dt);
			switch (c.blendMode)
			{
			case AnimationBlendMode::linear:
				UpdateLinear(c);
			case AnimationBlendMode::bezier:
				UpdateBezier(c);
			default:
				break;
			}
			weightSum += c.currentWeight;
		}

		// Normalize weights
		if(weightSum != 0)
			for (auto& c : ac.clips)
				c.currentWeight /= weightSum;

		// Tmp set matching norm time, required for run/walk etc.
		if (m_imguiMatching)
			ac.clips[2].normalizedTime = ac.clips[1].normalizedTime = ac.clips[0].normalizedTime;
	}
	void AnimationManager::UpdateBezier(AnimationComponent::AnimationClip& clip)
	{
		if (clip.normalizedTime > clip.transitionStart)
		{
			const f32 diff = clip.targetWeight - clip.currentWeight;
			const f32 tstart = clip.transitionStart;
			const f32 t = clip.normalizedTime - tstart;
			const f32 u = t / (clip.transitionTime);
			const f32 v = (1.0f - u);
			clip.currentWeight += diff * (3.f * v * u * u + std::powf(u, 3.f));
			clip.currentWeight = std::clamp(clip.currentWeight, 0.0f, 1.0f);
		}
	}
	void AnimationManager::UpdateLinear(AnimationComponent::AnimationClip& clip)
	{
		if (clip.normalizedTime > clip.transitionStart && clip.currentWeight != clip.targetWeight)
		{
			const f32 diff = clip.targetWeight - clip.currentWeight;
			const f32 tStart = clip.transitionStart;
			const f32 tCurrent = clip.normalizedTime - tStart;
			clip.currentWeight += diff * tCurrent / clip.transitionTime;
			clip.currentWeight = std::clamp(clip.currentWeight, 0.0f, 1.0f);
		}
	}

	DirectX::FXMVECTOR AnimationManager::CalculateScaling(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		// Scaling Weighted Average
		XMVECTOR scaleVec = XMVECTOR{};
		for (auto& c : ac.clips)
		{
			const auto& anim = rig.animations[c.animationID];
			scaleVec += c.currentWeight * GetKeyValue(anim.scaKeys.at(nodeID), KeyType::Scale, c.currentTick);
		}
		return scaleVec;
	}
	DirectX::FXMVECTOR AnimationManager::CalculateTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		// Translation Weighted Average
		XMVECTOR translationVec = XMVECTOR{};
		for (auto& c : ac.clips)
		{
			const auto& anim = rig.animations[c.animationID];
			translationVec += c.currentWeight * GetKeyValue(anim.posKeys.at(nodeID), KeyType::Translation, c.currentTick);
		}
		return translationVec;
	}
	DirectX::FXMVECTOR AnimationManager::CalculateRotation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
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

	void AnimationManager::UpdateMovementAnimation(DOG::AnimationComponent& ac, const f32 dt)
	{
		auto& clip0 = ac.clips[0];
		auto& clip1 = ac.clips[1];
		clip0.loop = true;
		clip1.loop = true;
		const auto absMS = abs(m_imguiMovementSpeed);

		if (absMS < 0.5f) // clip0 = walk, clip1 = idle
		{
			const auto& anim0 = m_rigs[0]->animations[2];
			const auto& anim1 = m_rigs[0]->animations[4];
			if (clip0.animationID != 4) clip0.SetAnimation(4, anim1.ticks, anim1.duration);
			if (clip1.animationID != 2) clip1.SetAnimation(2, anim0.ticks, anim0.duration);
			m_imguiMatching = true;
			clip1.currentWeight = 1.0f - absMS / 0.5f;
			clip0.currentWeight = 1.0f - clip1.currentWeight;
		}
		else // clip0 = walk, clip1 = run
		{
			const auto& anim0 = m_rigs[0]->animations[4];
			const auto& anim1 = m_rigs[0]->animations[5];
			if (clip0.animationID != 4) clip0.SetAnimation(4, anim0.ticks, anim0.duration, clip1.normalizedTime);
			if (clip1.animationID != 5) clip1.SetAnimation(5, anim1.ticks, anim1.duration, clip0.normalizedTime);
			m_imguiMatching = true;
			clip0.currentWeight = 1.0f - std::clamp(absMS-0.5f, 0.0f, 0.5f) * 2.0f;
			clip1.currentWeight = 1.0f - clip0.currentWeight;
			// Additional speed increases playback rate of animation
			clip0.timeScale = 1.0f + std::clamp(absMS - 0.5f, 0.0f, 2.f);
		}
		bool forward = m_imguiMovementSpeed > 0.f;
		bool positiv = clip0.timeScale > 0.f;
		if (forward != positiv)
		{
			clip0.timeScale *= -1.f;
			clip1.timeScale *= -1.f;
		}
		clip0.UpdateClip(dt);
		clip1.UpdateClip(dt);
		// Tmp set matching norm time, required for run/walk etc.
		if (m_imguiMatching)
			ac.clips[1].normalizedTime = ac.clips[0].normalizedTime;
	}

	void AnimationManager::UpdateMovementAnimation2(DOG::AnimationComponent& ac, const f32 dt)
	{
		auto& clip0 = ac.clips[0];
		auto& clip1 = ac.clips[1];
		clip0.loop = true;
		clip1.loop = true;
		const auto absMS = abs(m_imguiMovementSpeed);

		if (absMS < 0.5f) // clip0 = walk, clip1 = idle
		{
			const auto& anim0 = m_rigs[0]->animations[2];
			const auto& anim1 = m_rigs[0]->animations[4];
			if (clip0.animationID != 4) clip0.SetAnimation(4, anim1.ticks, anim1.duration);
			if (clip1.animationID != 2) clip1.SetAnimation(2, anim0.ticks, anim0.duration);
			m_imguiMatching = true;
			clip1.currentWeight = 1.0f - absMS / 0.5f;
			clip0.currentWeight = 1.0f - clip1.currentWeight;
		}
		else // clip0 = walk, clip1 = run
		{
			const auto& anim0 = m_rigs[0]->animations[4];
			const auto& anim1 = m_rigs[0]->animations[5];
			if (clip0.animationID != 4) clip0.SetAnimation(4, anim0.ticks, anim0.duration, clip1.normalizedTime);
			if (clip1.animationID != 5) clip1.SetAnimation(5, anim1.ticks, anim1.duration, clip0.normalizedTime);
			m_imguiMatching = true;
			clip0.currentWeight = 1.0f - std::clamp(absMS - 0.5f, 0.0f, 0.5f) * 2.0f;
			clip1.currentWeight = 1.0f - clip0.currentWeight;
			// Additional speed increases playback rate of animation
			clip0.timeScale = 1.0f + std::clamp(absMS - 0.5f, 0.0f, 2.f);
		}
		bool forward = m_imguiMovementSpeed > 0.f;
		bool positiv = clip0.timeScale > 0.f;
		if (forward != positiv)
		{
			clip0.timeScale *= -1.f;
			clip1.timeScale *= -1.f;
		}
		clip0.UpdateClip(dt);
		clip1.UpdateClip(dt);
		// Tmp set matching norm time, required for run/walk etc.
		if (m_imguiMatching)
			ac.clips[1].normalizedTime = ac.clips[0].normalizedTime;
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

	DirectX::FXMVECTOR AnimationManager::CalculateRotation2(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
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

}

