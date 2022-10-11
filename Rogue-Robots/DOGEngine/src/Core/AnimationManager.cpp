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
				UpdateClips(animatorC, (f32)Time::DeltaTime());
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
						ImGui::SliderInt(setName("    blendMode    ").c_str(), &clip.blendMode, 0, 4);
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
				}
				ImGui::Columns();
				if (ImGui::Button("Apply Clips")) // tmp
				{
					m_imguiPause = !m_imguiPause;
					imguiAnimC->clips[0] = clip0;
					imguiAnimC->clips[1] = clip1;
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
		const auto& animation0 = rig.animations[clip0.animationID];
		const auto& animation1 = rig.animations[clip1.animationID];

		const auto keyExists = [&nodeID](const std::unordered_map<i32, std::vector<AnimationKey>>& keys)
		{ return keys.find(nodeID) != keys.end(); };

		// Scale
		if (keyExists(animation0.scaKeys))
		{
			auto scale0 = GetKeyValue(animation0.scaKeys.at(nodeID), KeyType::Scale, t0);
			auto scale1 = GetKeyValue(animation1.scaKeys.at(nodeID), KeyType::Scale, t1);
			DirectX::XMStoreFloat3(&scaling, XMVectorLerp(scale0, scale1, 1.0f - clip1.currentWeight));
		}
		// Rotation
		if (keyExists(animation0.rotKeys))
		{
			auto rot1 = GetKeyValue(animation0.rotKeys.at(nodeID), KeyType::Rotation, t0);
			auto rot2 = GetKeyValue(animation1.rotKeys.at(nodeID), KeyType::Rotation, t1);
			DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(rot1, rot2, 1.0f - clip1.currentWeight));
		}
		// Translation
		const bool applyTranslation = (m_imguiRootTranslation || nodeID > m_rootBoneIdx);
		if (keyExists(animation0.posKeys) && applyTranslation)
		{
			auto translation1 = GetKeyValue(animation0.posKeys.at(nodeID), KeyType::Translation, t0);
			auto translation2 = GetKeyValue(animation1.posKeys.at(nodeID), KeyType::Translation, t1);
			DirectX::XMStoreFloat3(&translation, XMVectorLerp(translation1, translation2, 1.0f - clip1.currentWeight));
		}

		return XMMatrixTranspose(XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
			XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
			XMMatrixTranslation(translation.x, translation.y, translation.z));
	}

	void AnimationManager::UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::AnimationComponent& animator)
	{
		ZoneScopedN("skeletonUpdate");

		// Set node animation transformations
		std::vector<DirectX::XMMATRIX> hereditaryTFs;
		hereditaryTFs.reserve(rig.nodes.size());
		hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); ++i)
		{
			auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

			if(i < m_imguiMinMaskIdx || i > m_imguiMaxMaskIdx)
				ntf = CalculateBlendTransformation2(i, rig, animator);
			
#if defined _DEBUG
			DirectX::XMMATRIX imguiMatrix = DirectX::XMMatrixScaling(m_imguiSca[i].x, m_imguiSca[i].y, m_imguiSca[i].z) *
				DirectX::XMMatrixRotationRollPitchYaw(m_imguiRot[i].x, m_imguiRot[i].y, m_imguiRot[i].z) *
				DirectX::XMMatrixTranslation(m_imguiPos[i].x, m_imguiPos[i].y, m_imguiPos[i].z);
			ntf *= imguiMatrix;
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
				XMStoreFloat3(&scaling, XMLoadFloat4(&scalingKeys[0].value));
			else
				XMStoreFloat3(&scaling, GetKeyValue(animation.scaKeys.at(nodeID), KeyType::Scale, tick));
		}
		// Rotation
		if (keyExists(animation.rotKeys))
		{
			const auto& rotKeys = animation.rotKeys.at(nodeID);
			if (thereIsOnlyOneKeyIn(rotKeys))
				rotation = rotKeys[0].value;
			else
				XMStoreFloat4(&rotation, GetKeyValue(animation.rotKeys.at(nodeID), KeyType::Rotation, tick));
		}
		// Translation
		const bool applyTranslation = (m_imguiRootTranslation || nodeID > m_rootBoneIdx);
		if (keyExists(animation.posKeys) && applyTranslation)
		{
			const auto& posKeys = animation.posKeys.at(nodeID);
			if (thereIsOnlyOneKeyIn(posKeys))
				XMStoreFloat3(&translation, XMLoadFloat4(&posKeys[0].value));
			else
				DirectX::XMStoreFloat3(&translation, GetKeyValue(animation.posKeys.at(nodeID), KeyType::Translation, tick));
		}

		auto nodeTransform = XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
			XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
			XMMatrixTranslation(translation.x, translation.y, translation.z);

		return XMMatrixTranspose(nodeTransform);
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
		i32 key1Idx = key2Idx - 1;
		
		const auto& key1 = keys[key1Idx];
		const auto& key2 = keys[key2Idx];
		const auto blendFactor = (tick - key1.time) / (key2.time - key1.time);

		if (component == KeyType::Rotation)
			return XMQuaternionSlerp(XMLoadFloat4(&key1.value), XMLoadFloat4(&key2.value), blendFactor);
		else
			return XMVectorLerp(XMLoadFloat4(&key1.value), XMLoadFloat4(&key2.value), blendFactor);
	}

	void AnimationManager::UpdateClips(DOG::AnimationComponent& ac, const f32 dt)
	{
		static constexpr i32 linear = 1;
		static constexpr i32 belzier = 2;

		f32 weightSum = 0.0f;
		for (auto& c : ac.clips)
		{
			c.UpdateClip(dt);
			if (c.blendMode == linear)
				UpdateLinear(c, dt);
			else if (c.blendMode == belzier)
				UpdateBelzier(c, dt);
			weightSum += c.currentWeight;
		}

		// Normalize weights
		for (auto& c : ac.clips)
			c.currentWeight /= weightSum;

		// Tmp set matching norm time, required for run/walk etc.
		if (m_imguiMatching)
			ac.clips[1].normalizedTime = ac.clips[0].normalizedTime;
	}
	void AnimationManager::UpdateBelzier(AnimationComponent::AnimationClip& clip, const f32 dt)
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
	void AnimationManager::UpdateLinear(AnimationComponent::AnimationClip& clip, const f32 dt)
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
	
	void AnimationManager::UpdateComponent(const std::vector<DOG::AnimationData>& animations, DOG::AnimationComponent& ac, const f32 dt)
	{
	}

	DirectX::FXMMATRIX AnimationManager::CalculateBlendTransformation2(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::AnimationComponent& ac)
	{
		using namespace DirectX;
		XMFLOAT3 scaling = m_baseScale;
		XMFLOAT4 rotation = m_baseRotation;
		XMFLOAT3 translation = m_baseTranslation;

		const auto keyExists = [&nodeID](const std::unordered_map<i32, std::vector<AnimationKey>>& keys)
		{ return keys.find(nodeID) != keys.end(); };

		XMVECTOR scaleVec = XMVECTOR{};
		XMVECTOR transVec = XMVECTOR{};

		for (auto& c : ac.clips)
		{
			const auto& anim = rig.animations[c.animationID];
			// Weighted average for scaling/translation
			scaleVec += c.currentWeight * GetKeyValue(anim.scaKeys.at(nodeID), KeyType::Scale, c.currentTick);
			transVec += c.currentWeight * GetKeyValue(anim.posKeys.at(nodeID), KeyType::Translation, c.currentTick);
		}

		const auto& clip0 = ac.clips[0];
		const auto& clip1 = ac.clips[1];
		const auto& animation0 = rig.animations[clip0.animationID];
		const auto& animation1 = rig.animations[clip1.animationID];

		// Rotation
		if (keyExists(animation0.rotKeys))
		{
			auto rot0 = GetKeyValue(animation0.rotKeys.at(nodeID), KeyType::Rotation, clip0.currentTick);
			auto rot1 = GetKeyValue(animation1.rotKeys.at(nodeID), KeyType::Rotation, clip1.currentTick);
			DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(rot0, rot1, 1.0f - clip0.currentWeight));
		}

		DirectX::XMStoreFloat3(&scaling, scaleVec);
		const bool applyTranslation = (m_imguiRootTranslation || nodeID > m_rootBoneIdx);
		if (applyTranslation) DirectX::XMStoreFloat3(&translation, transVec);

		return XMMatrixTranspose(XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
			XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
			XMMatrixTranslation(translation.x, translation.y, translation.z));
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

