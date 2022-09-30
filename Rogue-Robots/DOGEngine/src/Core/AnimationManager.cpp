#include "AnimationManager.h"
#include "ImGUI/imgui.h"

AnimationManager::AnimationManager()
{
	m_imguiScale.assign(150, { 1.0f, 1.0f, 1.0f });
	m_imguiPos.assign(150, { 0.0f, 0.0f, 0.0f });
	m_imguiRot.assign(150, { 0.0f, 0.0f, 0.0f });
};

AnimationManager::~AnimationManager()
{

};

void AnimationManager::SpawnControlWindow()
{
	if (ImGui::Begin("BonneJoints (ppp;)"))
	{
		ImGui::Text("Perform update this many times (for profiling)");
		ImGui::SliderInt(" ", &m_imguiProfilePerformUpdate, 0, 100);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Columns(2, nullptr, true);
		if (ImGui::BeginCombo("animation1", m_rigs[0].animations[m_imguiAnimation].name.c_str()))
		{
			for (i32 i = 0; i < std::size(m_rigs[0].animations); i++)
				if (ImGui::Selectable((m_rigs[0].animations[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguiAnimation)))
					m_imguiAnimation = i;
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Animation Time", &m_imguiAnimTime, 0.0f, 1.0f, "%.5f");
		ImGui::SliderFloat("Time Scale", &m_imguiTimeScale, -2.0f, 2.0f, "%.3f");
		ImGui::Checkbox("Play Animation", &m_imguiPlayAnimation);
		ImGui::Checkbox("Bindpose", &m_imguiBindPose);
		ImGui::Checkbox("RootTranslation", &m_imguiRootTranslation);
		m_imguiPlayAnimation = m_imguiBindPose ? false : m_imguiPlayAnimation;

		if (m_imguiPlayAnimation)
			m_imguiAnimTime2 = m_imguiAnimTime;
		if (ImGui::BeginCombo("tfs", m_rigs[0].nodes[m_imguiSelectedBone].name.c_str()))
		{
			for (i32 i = 1; i < std::size(m_rigs[0].nodes); i++)
				if (ImGui::Selectable((m_rigs[0].nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imguiSelectedBone)))
					m_imguiSelectedBone = i;
			ImGui::EndCombo();
		}
		ImGui::Text("Orientation");
		ImGui::SliderAngle("Roll", &m_imguiRot[m_imguiSelectedBone].z, -180.0f, 180.0f);
		ImGui::SliderAngle("Pitch", &m_imguiRot[m_imguiSelectedBone].x, -180.0f, 180.0f);
		ImGui::SliderAngle("Yaw", &m_imguiRot[m_imguiSelectedBone].y, -180.0f, 180.0f);

		ImGui::Text("Translation");
		ImGui::SliderFloat("pos X", &m_imguiPos[m_imguiSelectedBone].x, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("pos Y", &m_imguiPos[m_imguiSelectedBone].y, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("pos Z", &m_imguiPos[m_imguiSelectedBone].z, -1.0f, 1.0f, "%.3f");

		ImGui::Text("Scale");
		ImGui::SliderFloat("X", &m_imguiScale[m_imguiSelectedBone].x, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Y", &m_imguiScale[m_imguiSelectedBone].y, -10.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Z", &m_imguiScale[m_imguiSelectedBone].z, -10.0f, 10.0f, "%.1f");
		ImGui::NextColumn();
		if (ImGui::BeginCombo("animation2", m_rigs[0].animations[m_imguiAnimation2].name.c_str()))
		{
			for (i32 j = 0; j < std::size(m_rigs[0].animations); j++)
				if (ImGui::Selectable((m_rigs[0].animations[j].name + " " + std::to_string(j)).c_str(), (j == m_imguiAnimation2)))
					m_imguiAnimation2 = j;
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Animation Time2", &m_imguiAnimTime2, 0.0f, 1.0f, "%.5f");
		ImGui::SliderFloat("blendFactor", &m_imguiBlend, 0.0f, 1.0f, "%.5f");
		ImGui::Checkbox("Blend", &m_imguiTestAnimationBlend);
	}
	ImGui::End();
}

DirectX::FXMMATRIX AnimationManager::CalculateBlendTransformation(i32 nodeID, const DOG::ImportedAnimation& rig, const DOG::AnimationComponent& ac)
{
	using namespace DirectX;
	XMFLOAT3 scaling = { 1.f, 1.f, 1.f };
	XMFLOAT3 translation = { 0.f, 0.f, 0.f };
	XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 0.f };

	f32 t1 = ac.tick[0];
	f32 t2 = ac.tick[1];
	const auto& animation1 = rig.animations[ac.animationID[0]];
	const auto& animation2 = rig.animations[ac.animationID[1]];

	if (animation1.scaKeys.find(nodeID) != animation1.scaKeys.end())
	{
		auto scale1 = GetAnimationComponent(animation1.scaKeys.at(nodeID), KeyType::Scale, t1);
		auto scale2 = GetAnimationComponent(animation2.scaKeys.at(nodeID), KeyType::Scale, t2);
		DirectX::XMStoreFloat3(&scaling, XMVectorLerp(scale1, scale2, /*tmp*/ac.bf));
	}
	if (animation1.rotKeys.find(nodeID) != animation1.rotKeys.end())
	{
		auto rot1 = GetAnimationComponent(animation1.rotKeys.at(nodeID), KeyType::Rotation, t1);
		auto rot2 = GetAnimationComponent(animation2.rotKeys.at(nodeID), KeyType::Rotation, t2);
		DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(rot1, rot2, ac.bf));
	}
	if (animation1.posKeys.find(nodeID) != animation1.posKeys.end() && (m_imguiRootTranslation || nodeID > 2))
	{
		auto translation1 = GetAnimationComponent(animation1.posKeys.at(nodeID), KeyType::Translation, t1);
		auto translation2 = GetAnimationComponent(animation2.posKeys.at(nodeID), KeyType::Translation, t2);
		DirectX::XMStoreFloat3(&translation, XMVectorLerp(translation1, translation2, ac.bf));
	}
	return XMMatrixTranspose(XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
		XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
		XMMatrixTranslation(translation.x, translation.y, translation.z));
}

DirectX::FXMMATRIX AnimationManager::CalculateBlendTransformation(i32 nodeID)
{
	using namespace DirectX;
	// Scaling
	XMFLOAT3 scaling = { 1.f, 1.f, 1.f };
	XMFLOAT3 translation = { 0.f, 0.f, 0.f };
	XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 0.f };

	f32 t1 = m_imguiAnimTime * m_rigs[0].animations[m_imguiAnimation].ticks;
	f32 t2 = m_imguiAnimTime2 * m_rigs[0].animations[m_imguiAnimation2].ticks;
	auto& animation1 = m_rigs[0].animations[m_imguiAnimation];
	auto& animation2 = m_rigs[0].animations[m_imguiAnimation2];

	if (animation1.scaKeys.find(nodeID) != animation1.scaKeys.end())
	{
		auto anim1 = GetAnimationComponent(animation1.scaKeys.at(nodeID), KeyType::Scale, t1);
		auto anim2 = GetAnimationComponent(animation2.scaKeys.at(nodeID), KeyType::Scale, t2);
		DirectX::XMStoreFloat3(&scaling, XMVectorLerp(anim1, anim2, m_imguiBlend));
	}
	if (animation1.rotKeys.find(nodeID) != animation1.rotKeys.end())
	{
		auto anim1 = GetAnimationComponent(animation1.rotKeys.at(nodeID), KeyType::Rotation, t1);
		auto anim2 = GetAnimationComponent(animation2.rotKeys.at(nodeID), KeyType::Rotation, t2);
		DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(anim1, anim2, m_imguiBlend));
	}
	if (animation1.posKeys.find(nodeID) != animation1.posKeys.end() && (m_imguiRootTranslation || nodeID > 2))
	{
		auto anim1 = GetAnimationComponent(animation1.posKeys.at(nodeID), KeyType::Translation, t1);
		auto anim2 = GetAnimationComponent(animation2.posKeys.at(nodeID), KeyType::Translation, t2);
		DirectX::XMStoreFloat3(&translation, XMVectorLerp(anim1, anim2, m_imguiBlend));
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
	if (animation.posKeys.find(nodeID) != animation.posKeys.end() && (m_imguiRootTranslation || nodeID > 2))
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

void AnimationManager::UpdateSkeleton(const DOG::AnimationComponent& animator, const DOG::ImportedAnimation& rig)
{
	// Set node animation transformations
	std::vector<DirectX::XMMATRIX> hereditaryTFs;
	hereditaryTFs.reserve(rig.nodes.size());
	hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
	for (i32 i = 1; i < rig.nodes.size(); i++)
	{
		auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

		if (!m_imguiBindPose && i > 1)
		{
			if (animator.animationID[0] != -1 && animator.animationID[1] != -1)
				ntf = CalculateBlendTransformation(i, rig, animator);
			else if(animator.animationID[0] != -1)
				ntf = CalculateNodeTransformation(rig.animations[animator.animationID[0]], i, animator.tick[0]);
			else if (animator.animationID[1] != -1)
				ntf = CalculateNodeTransformation(rig.animations[animator.animationID[1]], i, animator.tick[1]);
			else
				CalculateNodeTransformation(rig.animations[0], i, animator.tick[0]);
		}

		//imgui tf here
		hereditaryTFs.push_back(ntf);
	}
	// Apply parent Transformation
	for (size_t i = 1; i < hereditaryTFs.size(); i++)
		hereditaryTFs[i] = hereditaryTFs[rig.nodes[i].parentIdx] * hereditaryTFs[i];

	auto rootTF = DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation);
	for (size_t n = 0; n < rig.nodes.size(); n++)
	{
		auto joint = rig.nodes[n].jointIdx;
		if (joint != -1)
			DirectX::XMStoreFloat4x4(&m_vsJoints[animator.offset + joint],
				rootTF * hereditaryTFs[n] * DirectX::XMLoadFloat4x4(&rig.jointOffsets[joint]));
	}
}


void AnimationManager::UpdateSkeleton(u32 skeletonId, f32 dt)
{
	for (size_t m = 0; m < m_imguiProfilePerformUpdate; m++)
	{
		// For (job : jobs) get this data (imgui for now)
		const auto& rig = m_rigs[skeletonId];
		const auto& anim = rig.animations[m_imguiAnimation];
		if (m_imguiPlayAnimation)
		{
			m_imguiAnimTime += m_imguiTimeScale * dt / anim.duration;
			if (m_imguiAnimTime > 1.0f)
			{
				m_lastSRTkeys.assign(rig.nodes.size(), { 1,1,1 });
				while (m_imguiAnimTime > 1.0f)
					m_imguiAnimTime -= 1.0f;
			}
			else if (m_imguiAnimTime < 0.0f)
				m_imguiAnimTime = 1.0f + m_imguiAnimTime;
		}
		
		m_currentTick = m_imguiAnimTime * anim.ticks;

		std::vector<DirectX::XMMATRIX> hereditaryTFs;
		hereditaryTFs.reserve(rig.nodes.size());
		// Set node animation transformations
		hereditaryTFs.push_back(DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
		for (i32 i = 1; i < rig.nodes.size(); i++)
		{
			auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

			if (!m_imguiBindPose && i > 1)
				if (m_imguiTestAnimationBlend)
					ntf = CalculateBlendTransformation(i);
				else
					ntf = CalculateNodeTransformation(anim, i, m_currentTick);

			hereditaryTFs.push_back(ntf);
#if defined _DEBUG
			DirectX::XMMATRIX imguiMatrix = DirectX::XMMatrixScaling(m_imguiScale[i].x, m_imguiScale[i].y, m_imguiScale[i].z) *
				DirectX::XMMatrixRotationRollPitchYaw(m_imguiRot[i].x, m_imguiRot[i].y, m_imguiRot[i].z) *
				DirectX::XMMatrixTranslation(m_imguiPos[i].x, m_imguiPos[i].y, m_imguiPos[i].z);

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

void AnimationManager::UpdateAnimationComponent(DOG::AnimationComponent& ac, const std::vector<DOG::AnimationData>& animations, const f32 dt) const
{
	if (ac.mode == 0) // loop and blend with ImGui
	{
		for (u8 i = 0; i < MAX_ANIMATIONS; i++)
		{
			if (ac.animationID[i] == -1) // no Animation
				continue;
			const auto& animation = animations[ac.animationID[i]];
			ac.normalizedTime[i] += ac.timeScale[i] * dt / animation.duration;
			while (ac.normalizedTime[i] > 1.0f)
				ac.normalizedTime[i] -= 1.0f;
			if (ac.normalizedTime[i] < 0.0f)
				ac.normalizedTime[i] = 1.0f + ac.normalizedTime[i];

			ac.tick[i] = ac.normalizedTime[i] * animation.ticks;
		}
	}
	else if (ac.mode == 1) // something else
	{
		if (ac.animationID[0] == -1 || ac.animationID[1] == -1)
			return;
		const auto& anim1 = animations[ac.animationID[0]];
		const auto& anim2 = animations[ac.animationID[1]];

		ac.normalizedTime[0] += ac.timeScale[0] * dt / anim1.duration;
		if (ac.normalizedTime[0] > ac.transition)
			ac.normalizedTime[1] += ac.timeScale[1] * dt / anim2.duration;

		if (ac.normalizedTime[1] > 1.0f)
		{
			ac.normalizedTime[0] = 0.0f;
			ac.animationID[0] = -1;
			ac.mode = 0;
			return;
		}

		ac.bf = ac.normalizedTime[1]/(1.0f - ac.transition);
		ac.bf = std::clamp(ac.bf, 0.0f, 1.0f);
		ac.tick[0] = ac.normalizedTime[0] * anim1.ticks;
		ac.tick[1] = ac.normalizedTime[1] * anim2.ticks;
	}
}
