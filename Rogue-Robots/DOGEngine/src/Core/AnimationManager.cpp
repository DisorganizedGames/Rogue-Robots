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
		m_vsJoints.assign(300, {});
		ImGuiMenuLayer::RegisterDebugWindow("Animation Clip Setter", [this](bool& open) {SpawnControlWindow(open); });
	};

	AnimationManager::~AnimationManager()
	{
		ImGuiMenuLayer::UnRegisterDebugWindow("Animation Clip Setter");
	};

	void AnimationManager::Test(f32 dt)
	{
		static f32 deltaTime = 0.05f;
		static f32 timer = 0.f;
		static bool firstTime = true;
		static AnimationComponent testAc;
		if (!m_rigs.size())
			return;
		if (firstTime)
		{
			firstTime = false;
			mRigAnimator.rigData = m_rigs[MIXAMO_RIG_ID];
			static bool t1 = false, t2 = false, t3 = false;
			static AnimationComponent::Setter2 test1 = { true, 0, 0.0f, 1.0f, { 0, -1, -1}, { 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter2 test2 = { true, 2, 0.0f, 1.0f, { 2, -1, -1}, { 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter2 test3 = { false, 0, 0.5f, 1.0f, { 5, -1, -1},{ 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter2 test4 = { false, 0, 0.5f, 1.0f, { 6, -1, -1},{ 1.f, 0.f, 0.f} };

			testAc.addedSetters = 4;
			testAc.animSetters2[0] = test1;
			testAc.animSetters2[1] = test2;
			//testAc.animSetters2[2] = test3;
			//testAc.animSetters2[3] = test4;
			auto sz = sizeof(mRigAnimator);
			for (size_t i = 0; i < 4; i++)
			{
				static i8 bindIdx = 0, idleIdx = 2, walkIdx = 4;
				auto danceIdx = m_rigs[MIXAMO_RIG_ID]->animations.size() - i - 1;
				static AnimationComponent::Setter2 setter1 = { true, 0, 0.0f, 1.0f, { idleIdx, bindIdx, -1}, { 0.5f, 0.5f, 0.f} };
				static AnimationComponent::Setter2 setter2 = { true, 2, 0.0f, 1.0f, { 2, -1, -1}, { 1.f, 0.f, 0.f} };
				m_playerRigAnimators[i].rigData = m_rigs[MIXAMO_RIG_ID];
				testAc.addedSetters = 2;
				testAc.animSetters2[0] = setter1;
				testAc.animSetters2[1] = setter2;
				m_playerRigAnimators[i].HandleAnimationComponent(testAc);
			}
			mRigAnimator.HandleAnimationComponent(testAc);
		}
		timer += deltaTime;
		mRigAnimator.Update(deltaTime);
		auto stop = mRigAnimator.clipData[0];

		static bool secondTest = false;
		if (timer >= 0.6f && !secondTest)
		{
			AnimationComponent::Setter2 test = { false, 0, 0.0f, 1.0f, { 8, 9, -1}, { 0.35f, 0.15f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters2[0] = test;
			mRigAnimator.HandleAnimationComponent(testAc);
			secondTest = true;
		}
		static bool thirdTest = false;
		if (timer >= 1.3f && !thirdTest)
		{
			AnimationComponent::Setter2 test = { true, 1, 0.25f, 1.0f, { 3, 1, -1}, { 0.35f, 0.35f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters2[0] = test;
			mRigAnimator.HandleAnimationComponent(testAc);
			thirdTest = true;
		}
		/*if (timer >= 1.4f)
		{
			ExtractClipNodeInfluences(mRigAnimator, KeyType::Rotation, 0, MIXAMO_RIG_ID);
		}*/
	}

	void AnimationManager::UpdateJoints()
	{
		ZoneScopedN("updateJoints_ppp");
		using namespace DirectX;
		auto deltaTime = (f32)Time::DeltaTime();
		Test(0.1f);
		//tmp wtf
		static i32 count = 0;
		if (count < 3) {
			EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& modelaC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && modelaC.rigID == MIXAMO_RIG_ID && modelaC.animatorID == -1)
					{
						if(!m_rigs.size())
							m_rigs.push_back(&model->animation);

						modelaC.animatorID = GetNextAnimatorID();
						count++;
						// tmp setting base states
						auto idleIdx = 2, walkIdx = 0;
						auto danceIdx = m_rigs[modelaC.rigID]->animations.size() - modelaC.animatorID - 1;
						auto& dance = m_rigs[modelaC.rigID]->animations.rbegin()[modelaC.animatorID];
						auto& idle = m_rigs[modelaC.rigID]->animations[idleIdx];
						auto& walk = m_rigs[modelaC.rigID]->animations[walkIdx];
						auto& a = m_playerAnimators[modelaC.animatorID];
						a.AddAnimationClip(walkIdx, walk.duration, walk.ticks, 0, 0.f, 1.0f, 1.0f, true, 1.5f); // lower body walk
						a.AddAnimationClip(danceIdx, dance.duration, dance.ticks, 2, 0.f, 1.0f, 1.0f, true); // full body idle
					}
				});
			return;
		}
		
		// tmp
		auto mixamoCount = 0;

		EntityManager::Get().Collect<ModelComponent, AnimationComponent, TransformComponent>().Do([&](ModelComponent& modelC, AnimationComponent& rAC, TransformComponent& tfC)
			{
				ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
				if (model && rAC.animatorID != -1)
				{
					auto& rig = m_rigs[rAC.rigID];
					auto& animator = m_playerAnimators[rAC.animatorID];
					animator.offset = mixamoCount * MIXAMO_RIG.nJoints;
					rAC.offset = MIXAMO_RIG.nJoints * mixamoCount++;
					auto& addedAnimations = rAC.addedSetters;
					// Add animation clips for animations added this frame
					auto idx = 0;
					while(addedAnimations)
					{
						auto& s = rAC.animSetters[idx++];
						if (s.desired)
						{
							s.desired = false;
							animator.AddAnimationClip(
								s.animationID,
								rig->animations[s.animationID].duration,
								rig->animations[s.animationID].ticks,
								s.group,
								s.transitionLength,
								0.0f, // start weight
								1.0f, // target weight
								s.loop,
								s.playbackRate
							);
							f32 duration = rig->animations[s.animationID].duration / s.playbackRate;
							//f32 tl = duration / 6.f;
							animator.AddBlendSpecification(0.0f, s.transitionLength, s.group, 1.f, duration);
							--addedAnimations;
						}
					}

					animator.Update(deltaTime);
					auto& a = m_playerRigAnimators[rAC.animatorID];
					a.Update(deltaTime);
					//UpdateSkeleton(model->animation, animator);
					UpdateSkeleton(a, rAC.offset);
				}
			});
	}

	void AnimationManager::SpawnControlWindow(bool& open)
	{
		ZoneScopedN("animImgui3");
		static constexpr f32 m_imguiJointRotMin = -180.0f;
		static constexpr f32 m_imguiJointRotMax = 180.0f;
		static constexpr f32 m_imguiJointScaMin = -10.0f;
		static constexpr f32 m_imguiJointScaMax = 10.0f;
		static constexpr f32 m_imguiJointPosMin = -1.0f;
		static constexpr f32 m_imguiJointPosMax = 1.0f;

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
				// for now only Mixamo rig
				static u8 rigID = MIXAMO_RIG_ID;
				static std::string groupA = rigID == MIXAMO_RIG_ID ? "LowerBody" : "DoNotKnowYet";
				static std::string groupB = rigID == MIXAMO_RIG_ID ? "UpperBody" : "DoNotKnowYet";
				static std::string groupC = rigID == MIXAMO_RIG_ID ? "FullBody"  : "DoNotKnowYet";
				static auto& rig = m_rigs[rigID];
				static auto& anims = rig->animations;
				static AnimationComponent* imguiRAC;
				EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& rAC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && rAC.offset == 0)
						imguiRAC = &rAC;
				});

				static bool rigLoaded = m_rigs.size();
				if(!rigLoaded)
					return ImGui::End();

				static f32 animDuration = {};
				static i32 currAnim = 0;
				static bool displayDetails = false;

				if (ImGui::BeginCombo("animations", anims[currAnim].name.c_str()))
				{
					for (i32 i = 0; i < std::size(rig->animations); i++)
						if (ImGui::Selectable(("ID: " + std::to_string(i) + " = " + rig->animations[i].name).c_str(), (i == currAnim)))
							currAnim = i;
					ImGui::EndCombo();
				}

				if(displayDetails ^= ImGui::Button("DisplayDetails"))
				{
					static auto PrintTableRow = [](const std::string&& c1, const std::string&& c2){
						ImGui::TableNextColumn(); ImGui::Text(c1.c_str());
						ImGui::TableNextColumn(); ImGui::Text(c2.c_str());
					};
					if(ImGui::BeginTable(anims[currAnim].name.c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
					{
						auto& a = anims[currAnim]; // Combo Selected animation
						PrintTableRow("Name", a.name.c_str());
						PrintTableRow("Animation ID", std::to_string(currAnim));
						PrintTableRow("Duration", std::to_string(a.duration));
						PrintTableRow("Looping", (a.name.at(0) == '0' || a.name.at(0) == '1') ? "true" : "false");
						ImGui::EndTable();
					}
				}

				static bool setAnimationChain = false;
				if (setAnimationChain ^= ImGui::Button("SetAnimationChain"))
				{
				}
			}
			ImGui::End();
		}
			//	static AnimationComponent* imguiRAC;
			//	static bool rigLoaded = m_rigs.size();
			//	EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& rAC)
			//	{
			//		ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
			//		if (model && rAC.offset == 0)
			//		{
			//			imguiRAC = &rAC;
			//			if (!rigLoaded)
			//				m_rigs.push_back(&model->animation);
			//		}
			//		else
			//			return ImGui::End(); // "Animator";
			//	});
			//	return ImGui::End(); // "Animator";
			//	static i32 currAnim = 0;
			//	if (m_rigs[0]->animations.empty())
			//		auto ost = 0;
			
			//	return ImGui::End();

			//	// Some tmp imgui implementation for testing purposes
			//	static f32 playbackRate = 1.0f;
			//	static i32 transitionDiv = 6;
			//	static f32 animDuration = {};
			//	static u32 animID = {};
			//	static f32 cooldown = {};
			//	cooldown -= static_cast<f32>(Time::DeltaTime());
			//	static bool applyAnim = false;
			//	ImGui::Checkbox("RootTranslation", &m_imguiApplyRootTranslation);
			//	ImGui::SliderFloat("groupAWeight", &m_imguiGroupWeightA, 0.0f, 1.0f, "%.5f"); // tmp imgui controlling weight of groupA
			//	ImGui::SliderInt("Transition Div", &transitionDiv, 2, 10, "%.5f");
			//	ImGui::SliderFloat("playback rate", &playbackRate, 0.01f, 2.f, "%.5f");
			//	
			//	if (ImGui::Button("Grenade") && cooldown < 0.f)
			//	{
			//		applyAnim = true;
			//		animDuration = 3.23f;
			//		animID = 4;
			//	}
			//	if (ImGui::Button("Reload") && cooldown < 0.f)
			//	{
			//		applyAnim = true;
			//		animDuration = 3.33f;
			//		animID = 5;
			//	}
			//	if (ImGui::Button("Shoot") && cooldown < 0.f)
			//	{
			//		applyAnim = true;
			//		animDuration = 0.3f;
			//		animID = 6;
			//	}
			//	
			//	if (applyAnim)
			//	{
			//		const f32 duration = cooldown = animDuration / playbackRate;
			//		const f32 tl = duration / static_cast<f32>(transitionDiv);
			//		auto& setter = imguiRAC->animSetters[imguiRAC->addedSetters++];
			//		setter.animationID = static_cast<u8>(animID);
			//		setter.desired = true;
			//		setter.group = groupB;
			//		setter.loop = false;
			//		setter.transitionLength = tl;
			//		setter.playbackRate = playbackRate;
			//		//m_playerAnimators[0].AddAnimationClip(static_cast<i8>(animID), anim.duration, anim.ticks, groupB, tl, 0.f, 1.0f, false, playbackRate);
			//		//m_playerAnimators[0].AddBlendSpecification(0.0f, tl, groupB, 1.f, duration);
			//		applyAnim = false;
			//	}
			//	// ImGui individual joint sliders
			//	static i32 selectedBone = ROOT_NODE;
			//	if (ImGui::BeginCombo("tfs", m_rigs[0]->nodes[selectedBone].name.c_str()))
			//	{
			//		for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
			//			if (ImGui::Selectable((m_rigs[0]->nodes[i].name + "  " + std::to_string(i)).c_str(), (i == selectedBone)))
			//				selectedBone = i;
			//		ImGui::EndCombo();
			//	}
			//	
			//	ImGui::Text("Orientation");
			//	ImGui::SliderAngle("Roll", &m_imguiRot[selectedBone].z, m_imguiJointRotMin, m_imguiJointRotMax);
			//	ImGui::SliderAngle("Pitch", &m_imguiRot[selectedBone].x, m_imguiJointRotMin, m_imguiJointRotMax);
			//	ImGui::SliderAngle("Yaw", &m_imguiRot[selectedBone].y, m_imguiJointRotMin, m_imguiJointRotMax);
			//	ImGui::Text("Translation");
			//	ImGui::SliderFloat("pos X", &m_imguiPos[selectedBone].x, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
			//	ImGui::SliderFloat("pos Y", &m_imguiPos[selectedBone].y, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
			//	ImGui::SliderFloat("pos Z", &m_imguiPos[selectedBone].z, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
			//	ImGui::Text("Scale");
			//	ImGui::SliderFloat("X", &m_imguiSca[selectedBone].x, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
			//	ImGui::SliderFloat("Y", &m_imguiSca[selectedBone].y, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
			//	ImGui::SliderFloat("Z", &m_imguiSca[selectedBone].z, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
			//}
			//ImGui::End();
		//}
	}

	DirectX::FXMMATRIX AnimationManager::ImguiTransform(i32 i)
	{
		return DirectX::XMMatrixScaling(m_imguiSca[i].x, m_imguiSca[i].y, m_imguiSca[i].z) *
			DirectX::XMMatrixRotationRollPitchYaw(m_imguiRot[i].x, m_imguiRot[i].y, m_imguiRot[i].z) *
			DirectX::XMMatrixTranslation(m_imguiPos[i].x, m_imguiPos[i].y, m_imguiPos[i].z);
	}
	constexpr bool HasBone(const u32 v) {
		return v != -1;
	};

	void AnimationManager::UpdateSkeleton(const DOG::ImportedRig& rig, const DOG::Animator& animator)
	{
		ZoneScopedN("skeletonUpdate");
		using namespace DirectX;
		
		CalculateSRT(rig.animations, animator, MIXAMO_RIG_ID);
		// Set node animation transformations
		std::vector<XMMATRIX> hereditaryTFs;

		hereditaryTFs.reserve(rig.nodes.size());
		hereditaryTFs.push_back(XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); ++i)
		{
			auto ntf = XMLoadFloat4x4(&rig.nodes[i].transformation);
			
			// Compose pose matrix from keyframe values scale, rot, transl
			const auto sIdx = i * N_KEYS, rIdx = sIdx + 1, tIdx = sIdx + 2;
			ntf = XMMatrixTranspose(
				XMMatrixScalingFromVector(m_fullbodySRT[sIdx]) *
				XMMatrixRotationQuaternion(m_fullbodySRT[rIdx]) *
				XMMatrixTranslationFromVector(m_fullbodySRT[tIdx])
			);

#if defined _DEBUG
			// apply addition imgui bone influence
			ntf *= ImguiTransform(i);
#endif
			hereditaryTFs.push_back(ntf);
		}
		
		// Apply parent Transformation
		for (size_t i = 1; i < hereditaryTFs.size(); ++i)
			hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];
		// Finalize vertex shader joint matrix
		const auto rootTF = XMLoadFloat4x4(&rig.nodes[0].transformation);
		for (size_t n = 0; n < rig.nodes.size(); ++n)
		{
			auto joint = rig.nodes[n].jointIdx;
			if (HasBone(joint))
			{
				XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
					rootTF * hereditaryTFs[n] * XMLoadFloat4x4(&rig.jointOffsets[joint]));
			}
		}
	}

	DirectX::FXMVECTOR AnimationManager::GetKeyValue(const std::vector<DOG::AnimationKey>& keys, const KeyType& keyType, f32 tick)
	{
		using namespace DirectX;

		if (keys.size() == 1) // Only one key, return it
			return XMLoadFloat4(&keys[0].value);

		// tmp, will supply last key used in the future when key SRT values have been concatenated, for now this will have to do
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

		return (keyType == KeyType::Rotation) ?
			XMQuaternionSlerp(XMLoadFloat4(&key1.value), XMLoadFloat4(&key2.value), blendFactor) :
			XMVectorLerp(XMLoadFloat4(&key1.value), XMLoadFloat4(&key2.value), blendFactor);
	}

	DirectX::FXMVECTOR AnimationManager::ExtractRootTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::Animator& ac)
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

	void AnimationManager::CalculateSRT(const std::vector<AnimationData>& animations, const Animator& ac, const u8 rigID)
	{
		ZoneScopedN("SRT calculation");
		using namespace DirectX;
		const auto rigSpec = RIG_SPECIFICS[rigID];
		const auto nSRT = N_KEYS * NodeCount(rigID);
		// Store scaling, translation, rotation extracted from anim keyframes
		std::fill(m_partialSRT.begin(), m_partialSRT.begin() + nSRT, XMVECTOR{});
		std::fill(m_fullbodySRT.begin(), m_fullbodySRT.begin() + nSRT, XMVECTOR{});

		//const auto weightGroupA = ac.groupWeights[ac.groupA];
		const auto weightGroupA = m_imguiGroupWeightA; // tmp
		const auto weightGroupB = ac.groupWeights[ac.groupB];

		auto HasInfluence = [ac, rigID](const u8 group) {
			bool ret = ac.clipsInGroup[group];
			return ret && group == fullBodyGroup ? true : ac.groupWeights[group] > 0.f;
		};

		// Go through clip groups
		for (u8 group = 0; group < ac.nGroups; group++)
		{
			if (HasInfluence(group) || (ac.clipsInGroup[group] && group == groupA && weightGroupA>0.f))
			{
				const auto gClipIdx = ac.GetGroupIndex(group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Scale, ac.clipsInGroup[group], rigID, group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Rotation, ac.clipsInGroup[group], rigID, group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Translation, ac.clipsInGroup[group], rigID, group);
			}
		}

		// Blend between the partial body group and the full body animation group
		for (u8 i = 1; i < NodeCount(rigID); ++i)
		{
			f32 weight = 0.0f;
			if (i < rigSpec.rootJoint || InGroup(groupA, rigID, i)) // lower body
				weight = weightGroupA;
			else if (InGroup(groupB, rigID, i)) // upper body
				weight = weightGroupB;

			const u32 sIdx = i * 3, rIdx = i * 3 + 1, tIdx = i * 3 + 2;
			const XMVECTOR scaling1 = m_fullbodySRT[sIdx], scaling2 = m_partialSRT[sIdx];
			const XMVECTOR rotation1 = m_fullbodySRT[rIdx], rotation2 = m_partialSRT[rIdx];
			const XMVECTOR translation1 = m_fullbodySRT[tIdx], translation2 = m_partialSRT[tIdx];
			m_fullbodySRT[sIdx] = XMVectorLerp(scaling1, scaling2, weight);
			m_fullbodySRT[rIdx] = XMQuaternionSlerp(rotation1, rotation2, weight);
			m_fullbodySRT[tIdx] = XMVectorLerp(translation1, translation2, weight);
		}
	}

	void AnimationManager::ExtractClipNodeInfluences(const ClipData* cData, const std::vector<AnimationData>& anims, const KeyType key, const u8 nClips, const u8 rigID, const u8 group)
	{
		ZoneScopedN("Extract");
		using namespace DirectX;
		using AnimationKeys = std::unordered_map<i32, std::vector<AnimationKey>>;

		auto [startNode, nNodes] = GetNodeStartAndCount(rigID, group);

		const auto rootIdx = RIG_SPECIFICS[rigID].rootJoint;

		// First group controls root joint chain (tmp solution)
		group == groupA ? nNodes += rootIdx : nNodes;

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
				// Storage value
				const auto storeIdx = node * nClips;
				// Rig index
				auto rigNodeIdx = startNode + node;

				// First group controls root joint chain (tmp solution)
				if (group == groupA)
					rigNodeIdx = node < rootIdx ? node+1 : rigNodeIdx-rootIdx;
				
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

		auto& storeSRT = (group == fullBodyGroup) ? m_fullbodySRT :
													m_partialSRT;

		// Sum clip influences on each node for Weighted avg.
		for (u8 i = 0; i < nNodes; i++)
		{
			// index to first clip influencing bone
			const auto frstClipIdx = i * nClips;
			// rig node index
			auto rigNodeIdx = startNode + i;

			// First group controls root joint chain (tmp solution)
			if (group == groupA)
				rigNodeIdx = i < rootIdx ? i+1 : rigNodeIdx - rootIdx;

			const auto rigKeyIdx = N_KEYS * rigNodeIdx + static_cast<u32>(key);
			// Sum clip key values for weighted average
			if (key != KeyType::Rotation)
			{
				for (u8 j = 0; j < nClips; ++j)
				{
					const auto clipIdx = frstClipIdx + j;
					storeSRT[rigKeyIdx] += keyValues[clipIdx];
				}
				const bool rootDefault = key == KeyType::Translation && rigNodeIdx == ROOT_JOINT && !m_imguiApplyRootTranslation;
				// if no keyframe scaling/translation influence set base value
				if (XMComparisonAllTrue(XMVector3EqualR(storeSRT[rigKeyIdx], {})) || rootDefault)
				{
					const auto defaultValue = key == KeyType::Scale ? XMLoadFloat3(&m_baseScale) : XMLoadFloat3(&m_baseTranslation);
					storeSRT[rigKeyIdx] = defaultValue;
				}
			}
			else if(keyValues.size())
			{
				// weighted avg. for rot Quaternions
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

	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------
	// ---------- NEW RIG ---------

	void AnimationManager::UpdateSkeleton(DOG::RigAnimator& animator, const u32 offset)
	{
		ZoneScopedN("skeletonUpdate");
		using namespace DirectX;

		CalculateSRT(animator, MIXAMO_RIG_ID);
		// Set node animation transformations
		std::vector<XMMATRIX> hereditaryTFs;
		const auto& rig = animator.rigData;
		hereditaryTFs.reserve(rig->nodes.size());
		hereditaryTFs.push_back(XMLoadFloat4x4(&rig->nodes[0].transformation));
		for (i32 i = 1; i < rig->nodes.size(); ++i)
		{
			// Compose pose matrix from keyframe values scale, rot, and transl
			const auto sIdx = i * N_KEYS, rIdx = sIdx + 1, tIdx = sIdx + 2;
			auto ntf = XMMatrixTranspose(
				XMMatrixScalingFromVector(m_fullbodySRT[sIdx]) *
				XMMatrixRotationQuaternion(m_fullbodySRT[rIdx]) *
				XMMatrixTranslationFromVector(m_fullbodySRT[tIdx])
			);

#if defined _DEBUG
			// apply addition imgui bone influence
			ntf *= ImguiTransform(i);
#endif
			hereditaryTFs.push_back(ntf);
		}

		// Apply parent Transformation
		for (size_t i = 1; i < hereditaryTFs.size(); ++i)
			hereditaryTFs[i] = hereditaryTFs[rig->nodes[i].parentIdx] * hereditaryTFs[i];
		// Finalize vertex shader joint matrix
		const auto rootTF = XMLoadFloat4x4(&rig->nodes[0].transformation);
		for (size_t n = 0; n < rig->nodes.size(); ++n)
		{
			auto joint = rig->nodes[n].jointIdx;
			if (HasBone(joint))
			{
				XMStoreFloat4x4(&m_vsJoints[offset + joint],
					rootTF * hereditaryTFs[n] * XMLoadFloat4x4(&rig->jointOffsets[joint]));
			}
		}
	}

	void AnimationManager::CalculateSRT(RigAnimator& ac, const u8 rigID)
	{
		ZoneScopedN("SRT calculation");
		using namespace DirectX;
		const auto rigSpec = RIG_SPECIFICS[rigID];
		const auto nSRT = N_KEYS * NodeCount(rigID);
		// Store scaling, translation, rotation extracted from anim keyframes
		std::fill(m_partialSRT.begin(), m_partialSRT.begin() + nSRT, XMVECTOR{});
		std::fill(m_fullbodySRT.begin(), m_fullbodySRT.begin() + nSRT, XMVECTOR{});

		//const auto weightGroupA = ac.groupWeights[ac.groupA];
		const auto weightGroupA = 0.f; // tmp
		const auto weightGroupB = 0.f;

		auto HasInfluence = [ac](const u32 group) {
			bool ret = ac.groupClipCount[group] > 0;
			return ret;
		};

		std::unordered_map<u32, std::vector<XMVECTOR>> groupS;
		std::unordered_map<u32, std::vector<XMVECTOR>> groupR;
		std::unordered_map<u32, std::vector<XMVECTOR>> groupT;

		// Go through clip groups
		for (u32 group = 0; group < N_GROUPS; group++)
		{
			if (HasInfluence(group))
			{
				groupS.insert({ group, ExtractClipNodeInfluences(ac, KeyType::Scale, group, rigID) });
				groupR.insert({ group, ExtractClipNodeInfluences(ac, KeyType::Rotation, group, rigID) });
				groupT.insert({ group, ExtractClipNodeInfluences(ac, KeyType::Translation, group, rigID) });
			}
		}

		// TEEETST
		const auto& gsv = groupS[0];
		const auto& grv = groupR[0];
		const auto& gtv = groupT[0];
		for (u32 i = 0; i < groupS[0].size(); ++i)
		{
			const u32 sIdx = i * N_KEYS + static_cast<u32>(KeyType::Scale);
			const u32 rIdx = i * N_KEYS + static_cast<u32>(KeyType::Rotation);
			const u32 tIdx = i * N_KEYS + static_cast<u32>(KeyType::Translation);
			m_fullbodySRT[sIdx] = gsv[i];
			m_fullbodySRT[rIdx] = grv[i];
			m_fullbodySRT[tIdx] = gtv[i];
		}

		// Blend between the partial body group and the full body animation group
		for (u8 i = 1; i < NodeCount(rigID); ++i)
		{
			f32 weight = 0.0f;
			if (InGroup(groupA, rigID, i)) // lower body
				weight = weightGroupA;
			else if (InGroup(groupB, rigID, i)) // upper body
				weight = weightGroupB;

			const u32 sIdx = i * 3, rIdx = i * 3 + 1, tIdx = i * 3 + 2;
			const XMVECTOR scaling1 = m_fullbodySRT[sIdx], scaling2 = m_partialSRT[sIdx];
			const XMVECTOR rotation1 = m_fullbodySRT[rIdx], rotation2 = m_partialSRT[rIdx];
			const XMVECTOR translation1 = m_fullbodySRT[tIdx], translation2 = m_partialSRT[tIdx];
			m_fullbodySRT[sIdx] = XMVectorLerp(scaling1, scaling2, weight);
			m_fullbodySRT[rIdx] = XMQuaternionSlerp(rotation1, rotation2, weight);
			m_fullbodySRT[tIdx] = XMVectorLerp(translation1, translation2, weight);
		}
	}

	std::vector<DirectX::XMVECTOR> AnimationManager::ExtractClipNodeInfluences(RigAnimator& a, const KeyType key, const u32 group, const u32 rigID)
	{
		ZoneScopedN("Extract");
		using namespace DirectX;
		using PoseData = DOG::ClipData;
		using AnimationKeys = std::unordered_map<i32, std::vector<AnimationKey>>;

		auto [startNode, nNodes] = GetNodeStartAndCount(rigID, group);
		const auto rootIdx = RIG_SPECIFICS[rigID].rootJoint;
		const auto nClips = a.groupClipCount[group];
		const PoseData* clips = &a.clipData[a.GetGroupStartIdx(group)];
		const std::vector<AnimationData>& anims = a.rigData->animations;

		std::vector<XMVECTOR> keyValues(nClips * nNodes, XMVECTOR{});

		// For every clip in clip group
		for (u32 i = 0; i < nClips; i++)
		{
			// Pose data
			const auto aID = clips[i].aID;
			const auto weight = clips[i].weight;
			const auto tick = clips[i].tick;
			const auto& animation = anims[aID];
			// Store influence that the clip animation has on each node
			for (u32 node = 0; node < nNodes; node++)
			{
				// Storage value
				const auto storeIdx = node * nClips + i;
				// Rig index
				auto rigNodeIdx = startNode + node;

				const AnimationKeys* keys = {};
				switch (key)
				{
				case KeyType::Scale:
					keys = &animation.scaKeys;
					break;
				case KeyType::Rotation:
					keys = &animation.rotKeys;
					break;
				case KeyType::Translation:
					keys = &animation.posKeys;
					break;
				}
				// Keyframe influence exist, store it
				if (keys->find(rigNodeIdx) != keys->end())
				{
					auto keyVal = GetKeyValue(keys->at(rigNodeIdx), key, tick);
					keyValues[storeIdx] = key != KeyType::Rotation ? weight * keyVal : keyVal;
				}
			}
		}

		/*auto& storeSRT = group == RIG_SPECIFICS[rigID].fullbodyGroup ?
			m_fullbodySRT : m_partialSRT;*/
		std::vector<XMVECTOR> storeSRT(nNodes, XMVECTOR{});
		// Sum clip influences on each node for Weighted avg.
		for (u32 i = 0; i < nNodes; i++)
		{
			// index to first clip influencing bone
			const auto frstClipIdx = i * nClips;
			// rig node index
			auto nodeIdx = i;

			// First group controls root joint chain (tmp solution)
			if (group == groupA)
				nodeIdx = i < rootIdx ? i + 1 : nodeIdx - rootIdx;

			//const auto rigKeyIdx = N_KEYS * nodeIdx + static_cast<u32>(key);
			const auto rigKeyIdx = nodeIdx;
			
			if (key != KeyType::Rotation)
			{	// Sum clip key values for weighted average
				for (u32 j = 0; j < nClips; ++j)
				{
					const auto clipIdx = frstClipIdx + j;
					storeSRT[rigKeyIdx] += keyValues[clipIdx];
				}
				const bool rootDefault = key == KeyType::Translation && nodeIdx == ROOT_JOINT && !m_imguiApplyRootTranslation;
				// if no keyframe scaling/translation influence set base value
				if (XMComparisonAllTrue(XMVector3EqualR(storeSRT[rigKeyIdx], {})) || rootDefault)
				{
					const auto defaultValue = key == KeyType::Scale ? XMLoadFloat3(&m_baseScale) : XMLoadFloat3(&m_baseTranslation);
					storeSRT[rigKeyIdx] = defaultValue;
				}
			}
			else if (keyValues.size())
			{	// weighted avg. for rot Quaternions
				XMVECTOR q0 = keyValues[frstClipIdx];
				for (u32 j = 0; j < nClips; ++j)
				{
					const auto clipIdx = frstClipIdx + j;
					auto w = clips[j].weight;
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
		return storeSRT;
	}
}
