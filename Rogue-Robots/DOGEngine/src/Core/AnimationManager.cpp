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

		DOG::ImGuiMenuLayer::RegisterDebugWindow("RigJourno", std::bind(&AnimationManager::SpawnControlWindow, this, std::placeholders::_1), false, std::make_pair(DOG::Key::LCtrl, DOG::Key::A));
		m_ta.Update(0.4f);
	};

	AnimationManager::~AnimationManager()
	{
		ImGuiMenuLayer::UnRegisterDebugWindow("RigJourno");
	};

	void AnimationManager::ResetAnimationComponent(DOG::AnimationComponent& ac)
	{
		for (i32 i = 0; i < ac.addedSetters; ++i)
			for (i32 j = 0; j < ac.MAX_SETTERS; ++j)
				ac.animSetters[i].animationIDs[j] = NO_ANIMATION;
		ac.addedSetters = 0;
	}

	void AnimationManager::UpdateJoints()
	{
		ZoneScopedN("updateJoints_ppp");
		using namespace DirectX;
		auto deltaTime = (f32)Time::DeltaTime();

#ifdef _DEBUG
		deltaTime = m_imguiDeltaTime;
		//test(deltaTime);
#endif

		if (!m_rigs.size()) {
			EntityManager::Get().Collect<ModelComponent, RigDataComponent>().Do([&](ModelComponent& modelC, RigDataComponent& rC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (!m_rigs.size() && model && rC.rigID == MIXAMO_RIG_ID)
					{
						m_rigs.push_back(&model->animation);
						SetPlayerBaseStates();
					}
				});
			// Need to reset animationComponents that were set from other systems before rigs were loaded 
			EntityManager::Get().Collect<AnimationComponent>().Do([&](AnimationComponent& aC)
				{
					ResetAnimationComponent(aC);
				});
			return;
		}

		EntityManager::Get().Collect<AnimationComponent, MixamoHeadJointTF, TransformComponent>().Do([&](AnimationComponent& aC, MixamoHeadJointTF& jtf, TransformComponent& tf)
			{
				if (aC.animatorID != -1)
				{
					auto& a = m_playerRigAnimators[aC.animatorID];
					auto offset = MIXAMO_RIG.nJoints * aC.animatorID;
					a.Update(deltaTime);
					a.ProcessAnimationComponent(aC);
					UpdateSkeleton(a, offset);
					jtf.transform = SimpleMath::Matrix(XMMatrixTranslationFromVector(XMLoadFloat3(&m_headOffset)) *
						XMMatrixTranspose(XMLoadFloat4x4(&m_vsJoints[offset + MIXAMO_RIG.headJoint]))) * tf.worldMatrix;
				}
			});
	}

	bool ImGuiHandleArea(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, f32* legendWidth);
	void ImGuiTimeLine();

	void AnimationManager::SpawnControlWindow(bool& open)
	{
		ZoneScopedN("animImgui3");
		static constexpr f32 imguiJointRotMin = -180.0f;
		static constexpr f32 imguiJointRotMax = 180.0f;
		static constexpr f32 imguiJointScaMin = -10.0f;
		static constexpr f32 imguiJointScaMax = 10.0f;
		static constexpr f32 imguiJointPosMin = -1.0f;
		static constexpr f32 imguiJointPosMax = 1.0f;

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("RigJourno", "Ctrl+A"))
				open ^= true;
			ImGui::EndMenu(); // "View"
		}

		if (open && m_rigs.size())
		{
			ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("RigJourno", &open))
			{
				// for now only Mixamo rig
				static u8 rigID = MIXAMO_RIG_ID;
				static auto& rig = m_rigs[rigID];
				static auto& anims = rig->animations;

				if (!m_rigs.size())
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
				if (ImGui::Button("DisplayDetails"))
					displayDetails ^= 1;
				if (displayDetails)
				{
					static auto PrintTableRow = [](const std::string&& c1, const std::string&& c2) {
						ImGui::TableNextColumn(); ImGui::Text(c1.c_str());
						ImGui::TableNextColumn(); ImGui::Text(c2.c_str());
					};
					if (ImGui::BeginTable(anims[currAnim].name.c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
					{
						auto& a = anims[currAnim]; // Combo Selected animation
						PrintTableRow("Name", a.name.c_str());
						PrintTableRow("Animation ID", std::to_string(currAnim));
						PrintTableRow("Duration", std::to_string(a.duration));
						PrintTableRow("Looping", (a.name.at(0) == '0' || a.name.at(0) == '1') ? "true" : "false");
						ImGui::EndTable();
					}
				}

				// attempting to create timeline WIP
				//ImGuiTimeLine();

				// joint Transform component
				if (ImGui::BeginCombo("joint", m_rigs[0]->nodes[m_imguiJoint].name.c_str()))
				{
					for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
						if (ImGui::Selectable((m_rigs[0]->nodes[i].name).c_str(), (i == m_imguiJoint)))
							m_imguiJoint = i;
					ImGui::EndCombo();
				}
				static bool addClips = false;
#ifdef _DEBUG
				ImGui::SliderFloat("deltaT", &m_imguiDeltaTime, 0.001f, 0.1f, "%.3f");
#endif
				addClips ^= ImGui::Button("Add Clips");
				if (addClips)
				{
					static auto targets = 0;
					static const char* nTargets[]{ "One", "Two", "Three" };
					ImGui::Combo("n target clips", &targets, nTargets, IM_ARRAYSIZE(nTargets));

					static auto group = 0;
					static const char* grpNames[]{ "FullBody", "LowerBody", "UpperBody" };
					ImGui::Combo("target group", &group, grpNames, IM_ARRAYSIZE(grpNames));

					static auto playbackRate = 1.f, transitionLen = 0.f;
					ImGui::SliderFloat("playbackRate", &playbackRate, -1.f, 1.f, "%.2f");
					ImGui::SliderFloat("transitionLen", &transitionLen, 0.f, 1.f, "%.2f");

					static auto priority = static_cast<i32>(BASE_PRIORITY);
					ImGui::SliderInt("priority", &priority, 0, 5);

					static bool loopingFlag = false;
					ImGui::Checkbox("LoopingFlag", &loopingFlag);
					static bool persistFlag = false;
					ImGui::SameLine(); ImGui::Checkbox("PersistFlag", &persistFlag);
					static bool resetPrioFlag = false;
					ImGui::SameLine(); ImGui::Checkbox("ResetPrioFlag", &resetPrioFlag);

					ImGui::Columns(targets + 1);
					static i32 chosenAnims[MAX_TARGETS] = { 0 };
					static f32 weights[MAX_TARGETS] = { 0.f };
					for (i32 c = 0; c < targets + 1; ++c)
					{
						i32* a = &chosenAnims[c];
						if (ImGui::BeginCombo(("Clip#" + std::to_string(c)).c_str(), anims[*a].name.c_str()))
						{
							for (i32 i = 0; i < std::size(rig->animations); ++i)
								if (ImGui::Selectable(rig->animations[i].name.c_str(), (i == *a)))
									*a = i;
							ImGui::EndCombo();
						}
						ImGui::SliderFloat(("Weight#" + std::to_string(c)).c_str(), &weights[c], 0.f, 1.f, "%.2f");
						ImGui::NextColumn();
					}
					ImGui::Columns(1);
					if (ImGui::Button("Apply clip(s)"))
					{
						EntityManager::Get().Collect<ThisPlayer, AnimationComponent>().Do([&](ThisPlayer&, AnimationComponent& rAC)
							{
								if (rAC.animatorID == 0)
								{
									auto& s = rAC.animSetters[rAC.addedSetters++];
									s.playbackRate = playbackRate;
									if (persistFlag)
										s.flag = s.flag | AnimationFlag::Persist;
									if (loopingFlag)
										s.flag = s.flag | AnimationFlag::Looping;
									if (resetPrioFlag)
										s.flag = s.flag | AnimationFlag::ResetPrio;
									s.priority = static_cast<u8>(priority);
									s.group = static_cast<u8>(group);
									s.transitionLength = transitionLen;
									for (i32 i = 0; i < targets + 1; ++i)
									{
										s.animationIDs[i] = static_cast<i8>(chosenAnims[i]);
										s.targetWeights[i] = weights[i];
									}
								}

							});
					}
					ImGui::SameLine();
					if (ImGui::Button("SimpleAdd"))
					{
						EntityManager::Get().Collect<ThisPlayer, AnimationComponent>().Do([&](ThisPlayer&, AnimationComponent& rAC)
							{
								if (rAC.animatorID == 0)
								{
									AnimationFlag flg = AnimationFlag::None;
									if (persistFlag) flg = flg | AnimationFlag::Persist;
									if (loopingFlag) flg = flg | AnimationFlag::Looping;
									if (resetPrioFlag) flg = flg | AnimationFlag::ResetPrio;
									rAC.SimpleAdd(static_cast<i8>(chosenAnims[0]), flg);
								}

							});
					}
				}
				// ImGui individual joint sliders
				static bool individualJointSliders = false;
				individualJointSliders ^= ImGui::Button("Joint sliders");
				if (individualJointSliders)
				{
					static i32 selectedBone = ROOT_NODE;
					if (ImGui::BeginCombo("tfs", m_rigs[0]->nodes[selectedBone].name.c_str()))
					{
						for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
							if (ImGui::Selectable((m_rigs[0]->nodes[i].name + "  " + std::to_string(i)).c_str(), (i == selectedBone)))
								selectedBone = i;
						ImGui::EndCombo();
					}

					ImGui::Text("Orientation");
					ImGui::SliderAngle("Roll", &m_imguiRot[selectedBone].z, imguiJointRotMin, imguiJointRotMax);
					ImGui::SliderAngle("Pitch", &m_imguiRot[selectedBone].x, imguiJointRotMin, imguiJointRotMax);
					ImGui::SliderAngle("Yaw", &m_imguiRot[selectedBone].y, imguiJointRotMin, imguiJointRotMax);
					ImGui::Text("Translation");
					ImGui::SliderFloat("pos X", &m_imguiPos[selectedBone].x, imguiJointPosMin, imguiJointPosMax, "%.3f");
					ImGui::SliderFloat("pos Y", &m_imguiPos[selectedBone].y, imguiJointPosMin, imguiJointPosMax, "%.3f");
					ImGui::SliderFloat("pos Z", &m_imguiPos[selectedBone].z, imguiJointPosMin, imguiJointPosMax, "%.3f");
					ImGui::Text("Scale");
					ImGui::SliderFloat("X", &m_imguiSca[selectedBone].x, imguiJointScaMin, imguiJointScaMax, "%.1f");
					ImGui::SliderFloat("Y", &m_imguiSca[selectedBone].y, imguiJointScaMin, imguiJointScaMax, "%.1f");
					ImGui::SliderFloat("Z", &m_imguiSca[selectedBone].z, imguiJointScaMin, imguiJointScaMax, "%.1f");
				}
			}
			ImGui::End();
		}
	}
	// attempting to create timeline WIP
	void ImGuiTimeLine()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();

		//ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		static float legendWidth = 0.5f;
		ImVec4& legendBackground = style.Colors[ImGuiCol_FrameBg];
		ImVec2 legendSize = ImVec2{ canvasSize.x + legendWidth, canvasSize.y };
		drawList->AddRectFilled(canvasPos,
			{ canvasPos.x + legendSize.x, canvasPos.y + legendSize.y },
			ImColor(legendBackground)
		);

		ImGuiHandleArea(canvasPos, canvasSize, legendSize, &legendWidth);
	}

	// attempting to create timeline WIP
	bool ImGuiHandleArea(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, f32* legendWidth)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiStyle& style = ImGui::GetStyle();

		static bool hovering = false;
		static bool active = false;

		constexpr f32 splitterWidth = 16.f;
		const f32 swDiv2 = splitterWidth / 2.f;
		ImVec2 splitterBegin = { canvasPos.x + legendSize.x - swDiv2, canvasPos.y };
		if (ImGui::IsMouseHoveringRect(splitterBegin, ImVec2(splitterBegin.x + splitterWidth, splitterBegin.y + canvasSize.y)))
		{
			constexpr f32 splitterRenderWidth = 4.f;
			const ImVec4& splitterBgColor = style.Colors[ImGuiCol_FrameBgHovered];
			drawList->AddRectFilled(splitterBegin,
				ImVec2(splitterBegin.x + splitterRenderWidth, splitterBegin.y + canvasSize.y),
				ImColor(splitterBgColor)
			);
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			hovering = true;

			if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && !active)
			{
				active = true;
			}
		}
		else
		{
			hovering = false;
		}
		if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && active)
		{
			ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
			const f32 dragDist = ImGui::GetIO().MouseDelta.x;
			*legendWidth += dragDist;
		}
		else if (!ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && active)
		{
			ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = false;
			active = false;
		}
		return active;
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

	// !! Need this for future reference !!
	//DirectX::FXMVECTOR AnimationManager::ExtractRootTranslation(const i32 nodeID, const DOG::ImportedRig& rig, const DOG::Animator& ac)
	//{
	//	using namespace DirectX;
	//	// Translation Weighted Average
	//	XMVECTOR translationVec = XMVECTOR{};
	//	for (auto& c : ac.clips)
	//	{
	//		const auto& anim = rig.animations[c.animationID];
	//		if (anim.posKeys.find(nodeID) != anim.posKeys.end())
	//			translationVec += c.currentWeight * GetKeyValue(anim.posKeys.at(nodeID), KeyType::Translation, c.currentTick);
	//	}
	//	return translationVec;
	//}

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

			// apply addition imgui bone influence
			ntf *= ImguiTransform(i);

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

		// Number of srt key values
		const auto nSRT = N_KEYS * NodeCount(rigID);

		// Clear previous SRT data, to prep for storing new transformations
		std::fill(m_fullbodySRT.begin(), m_fullbodySRT.begin() + nSRT, XMVECTOR{});

		// Check if clip group influences final pose or not
		auto HasInfluence = [ac](const u32 group) {
			return group == fullBodyGroup || (ac.groupClipCount[group] > 0 && ac.groups[group].weight); };

		// Go through clip groups and update joint scale/rot/translation
		for (u32 i = 0; i < N_KEYS; i++)
		{
			const auto key = static_cast<KeyType>(i);
			for (u32 group = 0; group < N_GROUPS; ++group)
			{
				// Update joint transformations of rig if applicable
				if (HasInfluence(group))
					ExtractClipNodeInfluences(ac, key, group, rigID);
			}
		}
	}

	void AnimationManager::ExtractClipNodeInfluences(RigAnimator& a, const KeyType key, const u32 group, const u32 rigID)
	{
		ZoneScopedN("Extract");
		using namespace DirectX;
		using PoseData = DOG::ClipData;
		using AnimationKeys = std::unordered_map<i32, std::vector<AnimationKey>>;

		// Check if group has influence before performing calculations
		if (group != fullBodyGroup && a.GetGroupWeight(group) == 0.0f)
			return;

		// Start and number of jointNodes that group influences
		const auto [startNode, nNodes] = GetNodeStartAndCount(static_cast<u8>(rigID), static_cast<u8>(group));
		// Number of clips acting within group
		const auto nClips = a.groupClipCount[group];
		// Pointer to first clip in clip array that group influences
		const PoseData* clips = &a.clipData[a.GetGroupStartIdx(group)];
		// Animation data corresponding to rig
		const std::vector<AnimationData>& anims = a.rigData->animations;

		// Intermediary storage of extracted Keyvalues
		std::vector<XMVECTOR> keyValues(nClips * nNodes, XMVECTOR{});

		// For every clip in clip group
		for (u32 i = 0; i < static_cast<u32>(nClips); ++i)
		{
			// Clip Pose data
			const auto weight = clips[i].weight;
			const auto tick = clips[i].tick;
			const auto& animation = anims[clips[i].aID];
			// Store influence that the animation clip has on each node
			for (u32 node = 0; node < nNodes; ++node)
			{
				// Key value index
				const auto keyIdx = node * nClips + i;
				// Rig index
				auto rigNode = startNode + node;

				// Get corresponding key values
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
				if (keys->find(rigNode) != keys->end())
				{
					auto keyVal = GetKeyValue(keys->at(rigNode), key, tick);
					keyValues[keyIdx] = key != KeyType::Rotation ? weight * keyVal : keyVal;
				}
			}
		}
		// Store finalized transformation data (Could change this to be done 'in place' in keyValues Vector)
		std::array<XMVECTOR, NodeCount(MIXAMO_RIG_ID)> storeSRT = { XMVECTOR{} };

		// Sum clip influences on each node for Weighted avg.
		for (u32 i = 0; i < nNodes; ++i)
		{
			// index to first clip influencing bone
			auto clipIdx = i * nClips;

			if (key != KeyType::Rotation)
			{	// Sum clip key values for weighted average
				for (u32 j = 0; j < nClips; ++j, ++clipIdx)
					storeSRT[i] += keyValues[clipIdx];

				const bool rootDefault = key == KeyType::Translation && i == ROOT_JOINT && !m_imguiApplyRootTranslation;
				// if no keyframe scaling/translation influence set base value
				if (XMComparisonAllTrue(XMVector3EqualR(storeSRT[i], {})) || rootDefault)
					storeSRT[i] = key == KeyType::Scale ? XMLoadFloat3(&m_baseScale) : XMLoadFloat3(&m_baseTranslation);
			}
			else if (keyValues.size())
			{	// Different formula for rotation quaternions
				XMVECTOR q0 = keyValues[clipIdx];
				for (u32 j = 0; j < nClips; ++j, ++clipIdx)
				{	// Approx. weighted average of rotation quaternions
					auto w = clips[j].weight;
					XMVECTOR& q = keyValues[clipIdx];
					auto dot = XMVector4Dot(q0, q);
					if (j > 0 && dot.m128_f32[0] < 0.0)
						w = -w;
					storeSRT[i] += w * q;
				}
				storeSRT[i] = XMVector4Normalize(storeSRT[i]);

				// if no keyframe rotation influence set base rotation
				if (XMComparisonAllTrue(XMVector4EqualR(storeSRT[i], {})))
					storeSRT[i] = XMLoadFloat4(&m_baseRotation);
			}
		}

		// Unload transformation keys to final rig array
		for (u32 i = 0, node = startNode; i < nNodes; ++i, ++node)
		{
			// Index to rig array
			const auto idx = N_KEYS * node + static_cast<u32>(key);
			// Store in full body array, full body group is always run first
			if (group == fullBodyGroup)
				m_fullbodySRT[idx] = storeSRT[i];
			// The Following groups lerps/slerps their results with the previous results
			else
			{
				const auto weight = a.GetGroupWeight(group);
				m_fullbodySRT[idx] = key == KeyType::Rotation ?
					XMQuaternionSlerp(m_fullbodySRT[idx], storeSRT[i], weight) :
					XMVectorLerp(m_fullbodySRT[idx], storeSRT[i], weight);
			}
		}
	}

	// Set Base state of the player animators
	void AnimationManager::SetPlayerBaseStates()
	{
		AnimationComponent baseAc;
		static constexpr i8 idleIdx = 0;

		for (size_t i = 0; i < m_playerRigAnimators.size(); ++i)
		{
			baseAc.SimpleAdd(idleIdx, AnimationFlag::Looping | AnimationFlag::ResetPrio);
			m_playerRigAnimators[i].rigData = m_rigs[MIXAMO_RIG_ID];
			m_playerRigAnimators[i].ProcessAnimationComponent(baseAc);
			for (u32 j = 0; j < N_GROUPS; j++)
				m_playerRigAnimators[i].groups[j].parent = j == 0 ? -1 : 0;
		}
	}

	// Temporary but still useful debug code, magic variables galore
	void AnimationManager::Test(f32 dt)
	{
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
			static i8 bindIdx = 0, idleIdx = 2, walkIdx = 4;
			//const auto danceIdx = m_rigs[MIXAMO_RIG_ID]->animations.size() - 1;
			static AnimationComponent::Setter test1 = { AnimationFlag::Looping, 0, 0, 0.0f, 1.0f, { idleIdx, bindIdx, -1}, { 0.5f, 0.5f, 0.f} };
			static AnimationComponent::Setter test2 = { AnimationFlag::Looping, 0, 2, 0.0f, 1.0f, { 2, -1, -1}, { 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter test3 = { AnimationFlag::None, 0, 0, 0.5f, 1.0f, { 5, -1, -1},{ 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter test4 = { AnimationFlag::None, 0, 0, 0.5f, 1.0f, { 6, -1, -1},{ 1.f, 0.f, 0.f} };

			testAc.addedSetters = 2;
			testAc.animSetters[0] = test1;
			testAc.animSetters[1] = test2;
			//testAc.animSetters[2] = test3;
			//testAc.animSetters[3] = test4;
			//auto sz = sizeof(mRigAnimator);
			for (size_t i = 0; i < 4; i++)
			{
				//auto uniqueDanceIdx = danceIdx - i;
				m_playerRigAnimators[i].rigData = m_rigs[MIXAMO_RIG_ID];
				testAc.addedSetters = 2;
				testAc.animSetters[0] = test1;
				testAc.animSetters[1] = test2;
				m_playerRigAnimators[i].ProcessAnimationComponent(testAc);
			}
			testAc.addedSetters = 2;
			testAc.animSetters[0] = test1;
			testAc.animSetters[1] = test2;
			mRigAnimator.ProcessAnimationComponent(testAc);
		}
		timer += dt;
		mRigAnimator.Update(dt);
		auto stop = mRigAnimator.clipData[0];

		static bool secondTest = false;
		if (timer >= 0.6f && !secondTest)
		{
			AnimationComponent::Setter test = { AnimationFlag::None, 0, 0, 0.10f, 1.0f, { 8, 9, -1}, { 0.35f, 0.15f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters[0] = test;
			mRigAnimator.ProcessAnimationComponent(testAc);
			secondTest = true;
		}
		static bool thirdTest = false;
		if (timer >= 1.3f && !thirdTest)
		{
			AnimationComponent::Setter test = { AnimationFlag::None, 1, 0, 0.25f, 1.0f, { 3, 1, -1}, { 0.35f, 0.35f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters[0] = test;
			mRigAnimator.ProcessAnimationComponent(testAc);
			thirdTest = true;
		}
		static bool fourthTest = false;
		if (timer >= 5.f && !fourthTest)
		{
			for (size_t i = 0; i < 4; i++)
			{
				AnimationComponent::Setter test = { AnimationFlag::None, 0, 5, 0.25f, 1.0f, { 1, -1, -1}, { 0.35f, 0.35f, 0.f} };
				testAc.addedSetters = 1;
				testAc.animSetters[0] = test;
				m_playerRigAnimators[i].ProcessAnimationComponent(testAc);
			}
			fourthTest = true;
		}
	}
}
