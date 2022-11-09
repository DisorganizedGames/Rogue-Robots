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
			static i8 bindIdx = 0, idleIdx = 2, walkIdx = 4;
			const auto danceIdx = m_rigs[MIXAMO_RIG_ID]->animations.size() - 1;
			static AnimationComponent::Setter2 test1 = { true, 0, 0, 0.0f, 1.0f, { idleIdx, bindIdx, -1}, { 0.5f, 0.5f, 0.f} };
			static AnimationComponent::Setter2 test2 = { true, 0, 2, 0.0f, 1.0f, { 2, -1, -1}, { 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter2 test3 = { false, 0, 0, 0.5f, 1.0f, { 5, -1, -1},{ 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter2 test4 = { false, 0, 0, 0.5f, 1.0f, { 6, -1, -1},{ 1.f, 0.f, 0.f} };

			testAc.addedSetters = 2;
			testAc.animSetters2[0] = test1;
			testAc.animSetters2[1] = test2;
			//testAc.animSetters2[2] = test3;
			//testAc.animSetters2[3] = test4;
			auto sz = sizeof(mRigAnimator);
			for (size_t i = 0; i < 4; i++)
			{
				auto uniqueDanceIdx = danceIdx - i;
				m_playerRigAnimators[i].rigData = m_rigs[MIXAMO_RIG_ID];
				testAc.addedSetters = 2;
				testAc.animSetters2[0] = test1;
				testAc.animSetters2[1] = test2;
				m_playerRigAnimators[i].ProcessAnimationComponent(testAc);
			}
			testAc.addedSetters = 2;
			testAc.animSetters2[0] = test1;
			testAc.animSetters2[1] = test2;
			mRigAnimator.ProcessAnimationComponent(testAc);
		}
		timer += deltaTime;
		mRigAnimator.Update(deltaTime);
		auto stop = mRigAnimator.clipData[0];

		static bool secondTest = false;
		if (timer >= 0.6f && !secondTest)
		{
			AnimationComponent::Setter2 test = { false, 0, 0, 0.10f, 1.0f, { 8, 9, -1}, { 0.35f, 0.15f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters2[0] = test;
			mRigAnimator.ProcessAnimationComponent(testAc);
			secondTest = true;
		}
		static bool thirdTest = false;
		if (timer >= 1.3f && !thirdTest)
		{
			AnimationComponent::Setter2 test = { true, 1, 0, 0.25f, 1.0f, { 3, 1, -1}, { 0.35f, 0.35f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters2[0] = test;
			mRigAnimator.ProcessAnimationComponent(testAc);
			thirdTest = true;
		}
		static bool fourthTest = false;
		if (timer >= 5.f && !fourthTest)
		{
			for (size_t i = 0; i < 4; i++)
			{
				AnimationComponent::Setter2 test = { false, 0, 5, 0.25f, 1.0f, { 1, -1, -1}, { 0.35f, 0.35f, 0.f} };
				testAc.addedSetters = 1;
				testAc.animSetters2[0] = test;
				m_playerRigAnimators[i].ProcessAnimationComponent(testAc);
			}
			fourthTest = true;
		}
	}

	void AnimationManager::UpdateJoints()
	{
		ZoneScopedN("updateJoints_ppp");
		using namespace DirectX;
		auto deltaTime = (f32)Time::DeltaTime();
		//tmp debug stuff
		//deltaTime = 0.1f;
		//Test(deltaTime);
		if (!m_rigs.size()) {
			EntityManager::Get().Collect<ModelComponent, AnimationComponent>().Do([&](ModelComponent& modelC, AnimationComponent& modelaC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (!m_rigs.size() && model && modelaC.rigID == MIXAMO_RIG_ID)
					{
						m_rigs.push_back(&model->animation);
						SetPlayerBaseStates();
					}
				});
			return;
		}

		auto mixamoCount = 0;
		EntityManager::Get().Collect<AnimationComponent>().Do([&](AnimationComponent& rAC)
			{
				if (rAC.animatorID != -1)
				{
					auto& a = m_playerRigAnimators[rAC.animatorID];
					rAC.offset = MIXAMO_RIG.nJoints * mixamoCount++;
					a.Update(deltaTime);
					UpdateSkeleton(a, rAC.offset);
				}
			});
	}

	bool ImGuiHandleArea(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, f32* legendWidth);
	void ImGuiTimeLine();

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
				if (ImGui::Button("DisplayDetails"))
					displayDetails ^= 1;
				if(displayDetails)
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

				// attempting to create timeline WIP
				ImGuiTimeLine();
				/*static bool setAnimationChain = false;
				if (setAnimationChain ^= ImGui::Button("SetAnimationChain"))
				{
				}*/
			
				// ImGui individual joint sliders
				static i32 selectedBone = ROOT_NODE;
				if (ImGui::BeginCombo("tfs", m_rigs[0]->nodes[selectedBone].name.c_str()))
				{
					for (i32 i = 1; i < std::size(m_rigs[0]->nodes); i++)
						if (ImGui::Selectable((m_rigs[0]->nodes[i].name + "  " + std::to_string(i)).c_str(), (i == selectedBone)))
							selectedBone = i;
					ImGui::EndCombo();
				}
				
				ImGui::Text("Orientation");
				ImGui::SliderAngle("Roll", &m_imguiRot[selectedBone].z, m_imguiJointRotMin, m_imguiJointRotMax);
				ImGui::SliderAngle("Pitch", &m_imguiRot[selectedBone].x, m_imguiJointRotMin, m_imguiJointRotMax);
				ImGui::SliderAngle("Yaw", &m_imguiRot[selectedBone].y, m_imguiJointRotMin, m_imguiJointRotMax);
				ImGui::Text("Translation");
				ImGui::SliderFloat("pos X", &m_imguiPos[selectedBone].x, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
				ImGui::SliderFloat("pos Y", &m_imguiPos[selectedBone].y, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
				ImGui::SliderFloat("pos Z", &m_imguiPos[selectedBone].z, m_imguiJointPosMin, m_imguiJointPosMax, "%.3f");
				ImGui::Text("Scale");
				ImGui::SliderFloat("X", &m_imguiSca[selectedBone].x, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
				ImGui::SliderFloat("Y", &m_imguiSca[selectedBone].y, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
				ImGui::SliderFloat("Z", &m_imguiSca[selectedBone].z, m_imguiJointScaMin, m_imguiJointScaMax, "%.1f");
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

		// Number of srt key values
		const auto nSRT = N_KEYS * NodeCount(rigID);

		// Clear previous SRT data, to prep for storing new transformations
		std::fill(m_fullbodySRT.begin(), m_fullbodySRT.begin() + nSRT, XMVECTOR{});

		// Check if clip group influences final pose or not
		auto HasInfluence = [ac](const u32 group) {
			return group == fullBodyGroup || (ac.groupClipCount[group] > 0 && ac.groups[group].weight);};

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
		using Setter = DOG::AnimationComponent::Setter;
		// base state setter
		static constexpr bool loop = true;
		static constexpr u8 priority = 0;
		static constexpr f32 transitionLength = 0.f;
		static constexpr f32 playbackRate = 1.f;
		static constexpr i8 idleIdx = 2;
		static constexpr f32 weight = 1.f;
		Setter baseState = { loop, fullBodyGroup, priority, transitionLength, playbackRate,
			{ idleIdx, NO_ANIMATION, NO_ANIMATION },
			{ weight, 0.f, 0.f } };

		baseAc.addedSetters = 1;
		baseAc.animSetters[0] = baseState;

		for (size_t i = 0; i < m_playerRigAnimators.size(); ++i)
		{
			m_playerRigAnimators[i].rigData = m_rigs[MIXAMO_RIG_ID];
			m_playerRigAnimators[i].ProcessAnimationComponent(baseAc);
		}
		m_playerAnimatorsLoaded = true;
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
			static AnimationComponent::Setter test1 = { true, 0, 0, 0.0f, 1.0f, { idleIdx, bindIdx, -1}, { 0.5f, 0.5f, 0.f} };
			static AnimationComponent::Setter test2 = { true, 0, 2, 0.0f, 1.0f, { 2, -1, -1}, { 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter test3 = { false, 0, 0, 0.5f, 1.0f, { 5, -1, -1},{ 1.f, 0.f, 0.f} };
			static AnimationComponent::Setter test4 = { false, 0, 0, 0.5f, 1.0f, { 6, -1, -1},{ 1.f, 0.f, 0.f} };

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
			AnimationComponent::Setter test = { false, 0, 0, 0.10f, 1.0f, { 8, 9, -1}, { 0.35f, 0.15f, 0.f} };
			testAc.addedSetters = 1;
			testAc.animSetters[0] = test;
			mRigAnimator.ProcessAnimationComponent(testAc);
			secondTest = true;
		}
		static bool thirdTest = false;
		if (timer >= 1.3f && !thirdTest)
		{
			AnimationComponent::Setter test = { true, 1, 0, 0.25f, 1.0f, { 3, 1, -1}, { 0.35f, 0.35f, 0.f} };
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
				AnimationComponent::Setter test = { false, 0, 5, 0.25f, 1.0f, { 1, -1, -1}, { 0.35f, 0.35f, 0.f} };
				testAc.addedSetters = 1;
				testAc.animSetters[0] = test;
				m_playerRigAnimators[i].ProcessAnimationComponent(testAc);
			}
			fourthTest = true;
		}
	}
}
