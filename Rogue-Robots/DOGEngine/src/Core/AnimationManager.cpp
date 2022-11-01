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

	void AnimationManager::UpdateJoints()
	{
		ZoneScopedN("updateJoints_ppp");
		using namespace DirectX;
		auto deltaTime = (f32)Time::DeltaTime();
		//tmp setting base animaton
		static i32 count = 0;
		if (count < 3) {
			EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& modelaC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && modelaC.rigID == MIXAMO_RIG_ID && !m_playerAnimators[modelaC.animatorID].loaded)
					{
						if(!m_rigs.size())
							m_rigs.push_back(&model->animation);

						count++;
						// tmp setting base states cheat
						auto idleIdx = 2, walkIdx = 13, strafeLidx = 7, strafeRidx = 9, runIdx = 5, runBackIdx = 6;
						// state anims hack
						auto danceIdx = m_rigs[modelaC.rigID]->animations.size()-1 - modelaC.animatorID;
						auto& dance = m_rigs[modelaC.rigID]->animations[danceIdx];
						auto& idle = m_rigs[modelaC.rigID]->animations[idleIdx];
						auto& walk = m_rigs[modelaC.rigID]->animations[walkIdx];
						auto& strafeL = m_rigs[modelaC.rigID]->animations[strafeLidx];
						auto& strafeR = m_rigs[modelaC.rigID]->animations[strafeRidx];
						auto& runBack = m_rigs[modelaC.rigID]->animations[runBackIdx];

						auto& a = m_playerAnimators[modelaC.animatorID];
						a.offset = modelaC.animatorID * MIXAMO_RIG.nJoints;
						//a.AddAnimationClip(danceIdx, dance.duration, dance.ticks, 0, 0.f, 1.0f, 1.0f, true, 1.5f); // lower body walk
						//a.AddAnimationClip(danceIdx, dance.duration, dance.ticks, 2, 0.f, 1.0f, 1.0f, true); // full body idle

						a.AddAnimationClip(walkIdx, walk.duration, walk.ticks, 2, 0.f, 1.0f, 1.0f, true, 1.5f); // lower body walk
						a.AddAnimationClip(strafeLidx, strafeL.duration, strafeL.ticks, 2, 0.f, 1.0f, 1.0f, true, 1.5f); // lower body strafe
						a.AddAnimationClip(strafeRidx, strafeR.duration, strafeR.ticks, 2, 0.f, 1.0f, 1.0f, true, 1.5f); // lower body strafe
						a.AddAnimationClip(runBackIdx, runBack.duration, runBack.ticks, 2, 0.f, 1.0f, 1.0f, true, 1.0f); // lower body walk
						a.AddAnimationClip(idleIdx, idle.duration, idle.ticks, 2, 0.f, 1.0f, 1.0f, true); // full body idle
						auto& animator = m_playerAnimators[modelaC.animatorID];
						animator.loaded = true;
					}
				});
			return;
		}
		if (!m_rigs.size())
			return;

		// tmps
		static f32 timer = 0.0f;
		timer += deltaTime;
		static bool firstTime = true;
		auto mixamoCount = 0;

		EntityManager::Get().Collect<ModelComponent, AnimationComponent, TransformComponent>().Do([&](ModelComponent& modelC, AnimationComponent& rAC, TransformComponent& tfC)
			{
				ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
				if (model && rAC.animatorID != -1)
				{
					auto& rig = m_rigs[rAC.rigID];
					auto& animator = m_playerAnimators[rAC.animatorID];
					if (!animator.loaded)
						return;

					rAC.offset = animator.offset = rAC.animatorID *MIXAMO_RIG.nJoints;

					animator.HackUpdate(&rAC.input[0], deltaTime);
					UpdateSkeleton(model->animation, animator);
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
		const auto nSRT = N_KEYS * rigSpec.nNodes;
		// Store scaling, translation, rotation extracted from anim keyframes
		std::fill(m_partialSRT.begin(), m_partialSRT.begin() + nSRT, XMVECTOR{});
		std::fill(m_fullbodySRT.begin(), m_fullbodySRT.begin() + nSRT, XMVECTOR{});

		//const auto weightGroupA = ac.groupWeights[ac.groupA];
		auto weightGroupA = ac.groupWeights[ac.groupA];
		auto weightGroupB = ac.groupWeights[ac.groupB];
		weightGroupA = 0.f;
		weightGroupB = 0.f;

		auto HasInfluence = [ac, rigID](const u8 group) {
			bool ret = ac.clipsInGroup[group];
			return ret && group == RIG_SPECIFICS[rigID].fullbodyGroup ? true : ac.groupWeights[group] > 0.f;
		};

		auto why = ac.clipsInGroup[2];
		auto wh = ac.groupWeights[2];
		// Go through clip groups
		for (u8 group = 0; group < ac.nGroups; group++)
		{
			if (group == 2)
			{
				//const auto gClipIdx = ac.GetGroupIndex(group);
				auto gClipIdx = 0;
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Scale, ac.clipsInGroup[group], rigID, group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Rotation, ac.clipsInGroup[group], rigID, group);
				ExtractClipNodeInfluences(&ac.clipData[gClipIdx], animations, KeyType::Translation, ac.clipsInGroup[group], rigID, group);
			}
		}

		// Blend between the partial body group and the full body animation group
		for (u8 i = 1; i < rigSpec.nNodes; ++i)
		{
			f32 weight = 0.0f;
			if (i < rigSpec.rootJoint || InGroup(groupA, rigID, i)) // lower body
				weight = weightGroupA;
			else if (InGroup(groupB, rigID, i)) // upper body
				weight = weightGroupB;

			const u32 sIdx = i * 3, rIdx = i * 3 + 1, tIdx = i * 3 + 2;
			const XMVECTOR scaling1 = m_fullbodySRT[sIdx];
			const XMVECTOR rotation1 = m_fullbodySRT[rIdx];
			const XMVECTOR translation1 = m_fullbodySRT[tIdx];
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

		auto& storeSRT = group == RIG_SPECIFICS[rigID].fullbodyGroup ? 
													m_fullbodySRT : m_partialSRT;

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
}
