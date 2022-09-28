#include "AnimationManager.h"
#include "ImGUI/imgui.h"

AnimationManager::AnimationManager()
{
	m_imguiscale.assign(150, { 1.0f, 1.0f, 1.0f });
	m_imguipos.assign(150, { 0.0f, 0.0f, 0.0f });
	m_imguirot.assign(150, { 0.0f, 0.0f, 0.0f });
};

AnimationManager::~AnimationManager()
{

};

void AnimationManager::SpawnControlWindow()
{
	if (ImGui::Begin("BonneJoints (ppp;)"))
	{
		ImGui::Text("Perform update this many times (for profiling)");
		ImGui::SliderInt(" ", &m_imguiprofilePerformUpdate, 0, 100);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Columns(2, nullptr, true);
		if (ImGui::BeginCombo("animation1", m_rigs[0].animations[m_imguianimation].name.c_str()))
		{
			for (i32 i = 0; i < std::size(m_rigs[0].animations); i++)
				if (ImGui::Selectable((m_rigs[0].animations[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguianimation)))
					m_imguianimation = i;
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Animation Time", &m_imguianimTime, 0.0f, 1.0f, "%.5f");
		ImGui::SliderFloat("Time Scale", &m_imguitimeScale, -2.0f, 2.0f, "%.3f");
		ImGui::Checkbox("Play Animation", &m_imguiplayAnimation);
		ImGui::Checkbox("Bindpose", &m_imguibindPose);
		ImGui::Checkbox("RootTranslation", &m_imguirootTranslation);
		m_imguiplayAnimation = m_imguibindPose ? false : m_imguiplayAnimation;

		if (m_imguiplayAnimation)
			m_imguianimTime2 = m_imguianimTime;
		if (ImGui::BeginCombo("tfs", m_rigs[0].nodes[m_imguiselectedBone].name.c_str()))
		{
			for (i32 i = 1; i < std::size(m_rigs[0].nodes); i++)
				if (ImGui::Selectable((m_rigs[0].nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguiselectedBone)))
					m_imguiselectedBone = i;
			ImGui::EndCombo();
		}
		ImGui::Text("Orientation");
		ImGui::SliderAngle("Roll", &m_imguirot[m_imguiselectedBone].z, -180.0f, 180.0f);
		ImGui::SliderAngle("Pitch", &m_imguirot[m_imguiselectedBone].x, -180.0f, 180.0f);
		ImGui::SliderAngle("Yaw", &m_imguirot[m_imguiselectedBone].y, -180.0f, 180.0f);

		ImGui::Text("Translation");
		ImGui::SliderFloat("pos X", &m_imguipos[m_imguiselectedBone].x, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("pos Y", &m_imguipos[m_imguiselectedBone].y, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("pos Z", &m_imguipos[m_imguiselectedBone].z, -1.0f, 1.0f, "%.3f");

		ImGui::Text("Scale");
		ImGui::SliderFloat("X", &m_imguiscale[m_imguiselectedBone].x, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Y", &m_imguiscale[m_imguiselectedBone].y, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Z", &m_imguiscale[m_imguiselectedBone].z, -10.0f, 10.0f, "%.1f");
		ImGui::NextColumn();
		if (ImGui::BeginCombo("animation2", m_rigs[0].animations[m_imguianimation2].name.c_str()))
		{
			for (i32 j = 0; j < std::size(m_rigs[0].animations); j++)
				if (ImGui::Selectable((m_rigs[0].animations[j].name + " " + std::to_string(j)).c_str(), (j == m_imguianimation2)))
					m_imguianimation2 = j;
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Animation Time2", &m_imguianimTime2, 0.0f, 1.0f, "%.5f");
		ImGui::SliderFloat("blendFactor", &m_imguiblend, 0.0f, 1.0f, "%.5f");
		ImGui::Checkbox("Blend", &m_imguitestAnimationBlend);
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

	f32 t1 = m_imguianimTime * m_rigs[0].animations[m_imguianimation].ticks;
	f32 t2 = m_imguianimTime2 * m_rigs[0].animations[m_imguianimation2].ticks;
	auto& animation1 = m_rigs[0].animations[m_imguianimation];
	auto& animation2 = m_rigs[0].animations[m_imguianimation2];

	if (animation1.scaKeys.find(nodeID) != animation1.scaKeys.end())
	{
		auto anim1 = GetAnimationComponent(animation1.scaKeys.at(nodeID), KeyType::Scale, t1);
		auto anim2 = GetAnimationComponent(animation2.scaKeys.at(nodeID), KeyType::Scale, t2);
		DirectX::XMStoreFloat3(&scaling, XMVectorLerp(anim1, anim2, m_imguiblend));
	}
	if (animation1.rotKeys.find(nodeID) != animation1.rotKeys.end())
	{
		auto anim1 = GetAnimationComponent(animation1.rotKeys.at(nodeID), KeyType::Rotation, t1);
		auto anim2 = GetAnimationComponent(animation2.rotKeys.at(nodeID), KeyType::Rotation, t2);
		DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(anim1, anim2, m_imguiblend));
	}
	if (animation1.posKeys.find(nodeID) != animation1.posKeys.end() && (m_imguirootTranslation || nodeID > 2))
	{
		auto anim1 = GetAnimationComponent(animation1.posKeys.at(nodeID), KeyType::Translation, t1);
		auto anim2 = GetAnimationComponent(animation2.posKeys.at(nodeID), KeyType::Translation, t2);
		DirectX::XMStoreFloat3(&translation, XMVectorLerp(anim1, anim2, m_imguiblend));
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
	if (animation.posKeys.find(nodeID) != animation.posKeys.end() && (m_imguirootTranslation || nodeID > 2))
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
	for (size_t m = 0; m < m_imguiprofilePerformUpdate; m++)
	{
		// For (job : jobs) get this data (imgui for now)
		const auto& rig = m_rigs[skeletonId];
		const auto& anim = rig.animations[m_imguianimation];
		if (m_imguiplayAnimation)
		{
			m_imguianimTime += m_imguitimeScale * dt / anim.duration;
			if (m_imguianimTime > 1.0f)
			{
				m_lastSRTkeys.assign(rig.nodes.size(), { 1,1,1 });
				while (m_imguianimTime > 1.0f)
					m_imguianimTime -= 1.0f;
			}
			else if (m_imguianimTime < 0.0f)
				m_imguianimTime = 1.0f + m_imguianimTime;

			m_currentTick = m_imguianimTime * anim.ticks;
		}
		else
			m_currentTick = m_imguianimTime * anim.ticks;

		std::vector<DirectX::XMMATRIX> hereditaryTFs;
		hereditaryTFs.reserve(rig.nodes.size());
		// Set node animation transformations
		hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); i++)
		{
			auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

			if (!m_imguibindPose && i > 1)
				if (m_imguitestAnimationBlend)
					ntf = CalculateBlendTransformation(i);
				else
					ntf = CalculateNodeTransformation(anim, i, m_currentTick);

			hereditaryTFs.push_back(ntf);
#if defined _DEBUG
			DirectX::XMMATRIX imguiMatrix = DirectX::XMMatrixScaling(m_imguiscale[i].x, m_imguiscale[i].y, m_imguiscale[i].z) *
				DirectX::XMMatrixRotationRollPitchYaw(m_imguirot[i].x, m_imguirot[i].y, m_imguirot[i].z) *
				DirectX::XMMatrixTranslation(m_imguipos[i].x, m_imguipos[i].y, m_imguipos[i].z);

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