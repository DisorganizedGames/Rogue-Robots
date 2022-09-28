#include "AnimationManager.h"
#include "ImGUI/imgui.h"

AnimationManager::AnimationManager()
{
	m_imgui_scale.assign(150, { 1.0f, 1.0f, 1.0f });
	m_imgui_pos.assign(150, { 0.0f, 0.0f, 0.0f });
	m_imgui_rot.assign(150, { 0.0f, 0.0f, 0.0f });
};

AnimationManager::~AnimationManager()
{

};

void AnimationManager::SpawnControlWindow()
{
	if (ImGui::Begin("BonneJoints (ppp;)"))
	{
		ImGui::Text("Perform update this many times (for profiling)");
		ImGui::SliderInt(" ", &m_imgui_profilePerformUpdate, 0, 100);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Columns(2, nullptr, true);
		if (ImGui::BeginCombo("animation1", m_rigs[0].animations[m_imgui_animation].name.c_str()))
		{
			for (i32 i = 0; i < std::size(m_rigs[0].animations); i++)
				if (ImGui::Selectable((m_rigs[0].animations[i].name + "  " + std::to_string(i)).c_str(), (i == m_imgui_animation)))
					m_imgui_animation = i;
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Animation Time", &m_imgui_animTime, 0.0f, 1.0f, "%.5f");
		ImGui::SliderFloat("Time Scale", &m_imgui_timeScale, -2.0f, 2.0f, "%.3f");
		ImGui::Checkbox("Play Animation", &m_imgui_playAnimation);
		ImGui::Checkbox("Bindpose", &m_imgui_bindPose);
		ImGui::Checkbox("RootTranslation", &m_imgui_rootTranslation);
		m_imgui_playAnimation = m_imgui_bindPose ? false : m_imgui_playAnimation;

		if (m_imgui_playAnimation)
			m_imgui_animTime2 = m_imgui_animTime;
		if (ImGui::BeginCombo("tfs", m_rigs[0].nodes[m_imgui_selectedBone].name.c_str()))
		{
			for (i32 i = 1; i < std::size(m_rigs[0].nodes); i++)
				if (ImGui::Selectable((m_rigs[0].nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imgui_selectedBone)))
					m_imgui_selectedBone = i;
			ImGui::EndCombo();
		}
		ImGui::Text("Orientation");
		ImGui::SliderAngle("Roll", &m_imgui_rot[m_imgui_selectedBone].z, -180.0f, 180.0f);
		ImGui::SliderAngle("Pitch", &m_imgui_rot[m_imgui_selectedBone].x, -180.0f, 180.0f);
		ImGui::SliderAngle("Yaw", &m_imgui_rot[m_imgui_selectedBone].y, -180.0f, 180.0f);

		ImGui::Text("Translation");
		ImGui::SliderFloat("pos X", &m_imgui_pos[m_imgui_selectedBone].x, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("pos Y", &m_imgui_pos[m_imgui_selectedBone].y, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("pos Z", &m_imgui_pos[m_imgui_selectedBone].z, -1.0f, 1.0f, "%.3f");

		ImGui::Text("Scale");
		ImGui::SliderFloat("X", &m_imgui_scale[m_imgui_selectedBone].x, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Y", &m_imgui_scale[m_imgui_selectedBone].y, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Z", &m_imgui_scale[m_imgui_selectedBone].z, -10.0f, 10.0f, "%.1f");
		ImGui::NextColumn();
		if (ImGui::BeginCombo("animation2", m_rigs[0].animations[m_imgui_animation2].name.c_str()))
		{
			for (i32 j = 0; j < std::size(m_rigs[0].animations); j++)
				if (ImGui::Selectable((m_rigs[0].animations[j].name + " " + std::to_string(j)).c_str(), (j == m_imgui_animation2)))
					m_imgui_animation2 = j;
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Animation Time2", &m_imgui_animTime2, 0.0f, 1.0f, "%.5f");
		ImGui::SliderFloat("blendFactor", &m_imgui_blend, 0.0f, 1.0f, "%.5f");
		ImGui::Checkbox("Blend", &m_imgui_testAnimationBlend);
	}
	ImGui::End();
}

DirectX::FXMMATRIX AnimationManager::CalculateBlendTransformation(i32 nodeID)
{
	using namespace DirectX;
	// Scaling
	XMFLOAT3 scaling = { 1.f, 1.f, 1.f };
	XMFLOAT3 translation = { 0.f, 0.f, 0.f };
	XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 0.f };

	f32 t1 = m_imgui_animTime * m_rigs[0].animations[m_imgui_animation].ticks;
	f32 t2 = m_imgui_animTime2 * m_rigs[0].animations[m_imgui_animation2].ticks;
	auto& animation1 = m_rigs[0].animations[m_imgui_animation];
	auto& animation2 = m_rigs[0].animations[m_imgui_animation2];

	if (animation1.scaKeys.find(nodeID) != animation1.scaKeys.end())
	{
		auto anim1 = GetAnimationComponent(animation1.scaKeys.at(nodeID), KeyType::Scale, t1);
		auto anim2 = GetAnimationComponent(animation2.scaKeys.at(nodeID), KeyType::Scale, t2);
		DirectX::XMStoreFloat3(&scaling, XMVectorLerp(anim1, anim2, m_imgui_blend));
	}
	if (animation1.rotKeys.find(nodeID) != animation1.rotKeys.end())
	{
		auto anim1 = GetAnimationComponent(animation1.rotKeys.at(nodeID), KeyType::Rotation, t1);
		auto anim2 = GetAnimationComponent(animation2.rotKeys.at(nodeID), KeyType::Rotation, t2);
		DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(anim1, anim2, m_imgui_blend));
	}
	if (animation1.posKeys.find(nodeID) != animation1.posKeys.end() && (m_imgui_rootTranslation || nodeID > 2))
	{
		auto anim1 = GetAnimationComponent(animation1.posKeys.at(nodeID), KeyType::Translation, t1);
		auto anim2 = GetAnimationComponent(animation2.posKeys.at(nodeID), KeyType::Translation, t2);
		DirectX::XMStoreFloat3(&translation, XMVectorLerp(anim1, anim2, m_imgui_blend));
	}
	return XMMatrixTranspose(XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
		XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
		XMMatrixTranslation(translation.x, translation.y, translation.z));
}

DirectX::FXMMATRIX AnimationManager::CalculateNodeTransformation(const DOG::AnimationData& animation, i32 nodeID, f32 tick)
{
	using namespace DirectX;
	XMFLOAT3 scaling = { 1.f, 1.f, 1.f };
	XMFLOAT3 translation = { 0.f, 0.f, 0.f };
	XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 0.f };

	// Scaling
	if (animation.scaKeys.find(nodeID) != animation.scaKeys.end())
	{
		const auto& keys = animation.scaKeys.at(nodeID);
		if (keys.size() == 1)
			scaling = { keys[0].value.x, keys[0].value.y, keys[0].value.z };
		else
			DirectX::XMStoreFloat3(&scaling, GetAnimationComponent(animation.scaKeys.at(nodeID), KeyType::Scale, tick));
	}
	// Rotation
	if (animation.rotKeys.find(nodeID) != animation.rotKeys.end())
	{
		const auto& keys = animation.rotKeys.at(nodeID);
		if (keys.size() == 1)
			rotation = { keys[0].value.x, keys[0].value.y, keys[0].value.z, keys[0].value.w };
		else
			DirectX::XMStoreFloat4(&rotation, GetAnimationComponent(animation.rotKeys.at(nodeID), KeyType::Rotation, tick));
	}
	// Translation
	if (animation.posKeys.find(nodeID) != animation.posKeys.end() && (m_imgui_rootTranslation || nodeID > 2))
	{
		const auto& keys = animation.posKeys.at(nodeID);
		if (keys.size() == 1)
			translation = { keys[0].value.x, keys[0].value.y, keys[0].value.z };
		else
			DirectX::XMStoreFloat3(&translation, GetAnimationComponent(animation.posKeys.at(nodeID), KeyType::Translation, tick));
	}

	auto nodeTransform = XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
		XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
		XMMatrixTranslation(translation.x, translation.y, translation.z);

	return XMMatrixTranspose(nodeTransform);
}

void AnimationManager::UpdateSkeleton(u32 skeletonId, f32 dt)
{
	for (size_t m = 0; m < m_imgui_profilePerformUpdate; m++)
	{
		// For (job : jobs) get this data (imgui for now)
		const auto& rig = m_rigs[skeletonId];
		const auto& anim = rig.animations[m_imgui_animation];
		if (m_imgui_playAnimation)
		{
			m_imgui_animTime += m_imgui_timeScale * dt / anim.duration;
			if (m_imgui_animTime > 1.0f)
			{
				m_lastSRTkeys.assign(rig.nodes.size(), { 1,1,1 });
				while (m_imgui_animTime > 1.0f)
					m_imgui_animTime -= 1.0f;
			}
			else if (m_imgui_animTime < 0.0f)
				m_imgui_animTime = 1.0f + m_imgui_animTime;

			m_currentTick = m_imgui_animTime * anim.ticks;
		}
		else
			m_currentTick = m_imgui_animTime * anim.ticks;


		std::vector<DirectX::XMMATRIX> hereditaryTFs;
		hereditaryTFs.reserve(rig.nodes.size());
		// Set node animation transformations
		hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); i++)
		{
			auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

			if (!m_imgui_bindPose && i > 1)
				if (m_imgui_testAnimationBlend)
					ntf = CalculateBlendTransformation(i);
				else
					ntf = CalculateNodeTransformation(anim, i, m_currentTick);

			hereditaryTFs.push_back(ntf);
#if defined _DEBUG
			DirectX::XMMATRIX imguiMatrix = DirectX::XMMatrixScaling(m_imgui_scale[i].x, m_imgui_scale[i].y, m_imgui_scale[i].z) *
				DirectX::XMMatrixRotationRollPitchYaw(m_imgui_rot[i].x, m_imgui_rot[i].y, m_imgui_rot[i].z) *
				DirectX::XMMatrixTranslation(m_imgui_pos[i].x, m_imgui_pos[i].y, m_imgui_pos[i].z);

			hereditaryTFs.back() *= imguiMatrix;
#endif
		}

		// Apply parent Transformation
		for (size_t i = 1; i < hereditaryTFs.size(); i++)
			hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

		auto rootTF = DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation);
		for (size_t n = 0; n < rig.nodes.size(); n++)
		{
			auto joint = rig.nodes[n].jointIdx;
			if (joint != -1)
				DirectX::XMStoreFloat4x4(&m_vsJoints[joint], rootTF * hereditaryTFs[n] * DirectX::XMLoadFloat4x4(&rig.jointOffsets[joint]));
		}
	}
};
void AnimationManager::SetImportedAnimations(DOG::ImportedAnimation& ia)
{
	m_rigs.push_back(ia);
	for (auto& om : ia.jointOffsets)
		m_vsJoints.push_back(om);
};

DirectX::XMVECTOR AnimationManager::GetAnimationComponent(const std::vector<DOG::AnimationKey>& keys, KeyType component, f32 tick)
{
	using namespace DirectX;

	if (keys.size() == 1)
		return XMLoadFloat4(&keys[0].value);

	// Dirty, animations should supply last key used 
	i32 key2Idx = 1;
	while (key2Idx < keys.size() - 1 && keys[key2Idx].time < tick)
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