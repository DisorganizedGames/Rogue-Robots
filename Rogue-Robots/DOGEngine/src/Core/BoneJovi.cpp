#include "BoneJovi.h"

BoneJovi::BoneJovi()
{

};

BoneJovi::~BoneJovi()
{

};

//DirectX::FXMMATRIX BoneJovi::CalculateBlendTransformation(i32 nodeID)
//{
//	using namespace DirectX;
//	// Scaling
//	XMFLOAT3 scaling = { 1.f, 1.f, 1.f };
//	XMFLOAT3 translation = { 0.f, 0.f, 0.f };
//	XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 0.f };
//
//	f32 t1 = m_imgui_animTime * m_animations[m_imgui_animation].ticks;
//	f32 t2 = m_imgui_animTime2 * m_animations[m_imgui_animation2].ticks;
//
//	if (m_animations[m_imgui_animation].scaKeys.find(nodeID) != m_animations[m_imgui_animation].scaKeys.end())
//	{
//		auto anim1 = GetAnimationComponent(m_imgui_animation, KeyType::Scale, nodeID, t1);
//		auto anim2 = GetAnimationComponent(m_imgui_animation2, KeyType::Scale, nodeID, t2);
//		DirectX::XMStoreFloat3(&scaling, XMVectorLerp(anim1, anim2, m_imgui_blend));
//	}
//	if (m_animations[m_imgui_animation].rotKeys.find(nodeID) != m_animations[m_imgui_animation].rotKeys.end())
//	{
//		auto anim1 = GetAnimationComponent(m_imgui_animation, KeyType::Rotation, nodeID, t1);
//		auto anim2 = GetAnimationComponent(m_imgui_animation2, KeyType::Rotation, nodeID, t2);
//		DirectX::XMStoreFloat4(&rotation, XMQuaternionSlerp(anim1, anim2, m_imgui_blend));
//	}
//	if (m_animations[m_imgui_animation].posKeys.find(nodeID) != m_animations[m_imgui_animation].posKeys.end() && (m_imgui_rootTranslation || nodeID > 2))
//	{
//		auto anim1 = GetAnimationComponent(m_imgui_animation, KeyType::Translation, nodeID, t1);
//		auto anim2 = GetAnimationComponent(m_imgui_animation2, KeyType::Translation, nodeID, t2);
//		DirectX::XMStoreFloat3(&translation, XMVectorLerp(anim1, anim2, m_imgui_blend));
//	}
//	return XMMatrixTranspose(XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
//		XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
//		XMMatrixTranslation(translation.x, translation.y, translation.z));
//}
DirectX::FXMMATRIX BoneJovi::CalculateNodeTransformation(const DOG::AnimationData& animation, i32 nodeID, f32 tick)
{
	using namespace DirectX;
	XMFLOAT3 scaling = { 1.f, 1.f, 1.f };
	XMFLOAT3 translation = { 0.f, 0.f, 0.f };
	XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 0.f };

	auto getKeys = [](i32 idx, f32 tick, std::vector<DOG::AnimationKey>& keys, bool stupid = true)
	{
		if (keys.size() == 2)
			return std::pair<i32, i32>{0, 1};
		if (stupid) // For debugging (not using last known srt keys) funny bug not working :(
			idx = 0;
		while (idx < keys.size() - 1 && keys[idx].time < tick)
			idx++;
		idx = std::clamp(idx, 1, i32(keys.size() - 1));
		return std::pair<i32, i32>{idx - 1, idx};
	};
	auto x = m_animations[0].nodes;
	// Scaling
	if (animation.scaKeys.find(nodeID) != animation.scaKeys.end())
	{
		const auto& keys = animation.scaKeys.at(nodeID);
		if (keys.size() == 1)
			scaling = { keys[0].value.x, keys[0].value.y, keys[0].value.z };
		else
		{
			DirectX::XMStoreFloat3(&scaling, GetAnimationComponent(animation, KeyType::Scale, nodeID, tick));
			/*auto k1k2 = getKeys(m_lastSRTkeys[nodeID].srt[0], tick, animation.scaKeys.at(nodeID));
			DirectX::XMStoreFloat3(&scaling,
				XMVectorLerp(XMLoadFloat4(&keys[k1k2.first].value), XMLoadFloat4(&keys[k1k2.second].value),
					(tick - keys[k1k2.first].time) / (keys[k1k2.second].time - keys[k1k2.first].time)));
			m_lastSRTkeys[nodeID].srt[0] = k1k2.first;*/
		}
	}
	// Rotation
	if (animation.rotKeys.find(nodeID) != animation.rotKeys.end())
	{
		const auto& keys = animation.rotKeys.at(nodeID);
		if (keys.size() == 1)
			rotation = { keys[0].value.x, keys[0].value.y, keys[0].value.z, keys[0].value.w };
		else
		{
			DirectX::XMStoreFloat4(&rotation, GetAnimationComponent(animation, KeyType::Rotation, nodeID, tick));
			/*auto k1k2 = getKeys(m_lastSRTkeys[nodeID].srt[1], tick, animation.rotKeys.at(nodeID));
			DirectX::XMStoreFloat4(&rotation,
				XMQuaternionSlerp(XMLoadFloat4(&keys[k1k2.first].value), XMLoadFloat4(&keys[k1k2.second].value),
					(tick - keys[k1k2.first].time) / (keys[k1k2.second].time - keys[k1k2.first].time)));
			m_lastSRTkeys[nodeID].srt[1] = k1k2.first;*/
		}
	}
	// Translation
	if (animation.posKeys.find(nodeID) != animation.posKeys.end() && (m_imgui_rootTranslation || nodeID > 2))
	{
		const auto& keys = animation.posKeys.at(nodeID);
		if (keys.size() == 1)
			translation = { keys[0].value.x, keys[0].value.y, keys[0].value.z };
		else
		{
			DirectX::XMStoreFloat3(&translation, GetAnimationComponent(animation, KeyType::Translation, nodeID, tick));
			/*auto k1k2 = getKeys(m_lastSRTkeys[nodeID].srt[2], tick, m_animations[animation].posKeys.at(nodeID));
			DirectX::XMStoreFloat3(&translation,
				XMVectorLerp(XMLoadFloat4(&keys[k1k2.first].value), XMLoadFloat4(&keys[k1k2.second].value),
					(tick - keys[k1k2.first].time) / (keys[k1k2.second].time - keys[k1k2.first].time)));
			m_lastSRTkeys[nodeID].srt[2] = k1k2.first;*/
		}
	}

	auto nodeTransform = XMMatrixScaling(scaling.x, scaling.y, scaling.z) *
		XMMatrixRotationQuaternion(XMQuaternionNormalize(DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w))) *
		XMMatrixTranslation(translation.x, translation.y, translation.z);

	return XMMatrixTranspose(nodeTransform);
}

void BoneJovi::UpdateSkeleton(u32 skeletonId, u32 animId, f32 dt)
{
	const auto& rig = m_animations[skeletonId];
	const auto& anim = rig.animations[animId];
	// For (job : jobs) get this data
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
	hereditaryTFs.push_back(DirectX::XMMatrixScaling(0.25f, 0.25f, 0.25f) * DirectX::XMLoadFloat4x4(&rig.nodes[0].transformation));
	for (i32 i = 1; i < rig.nodes.size(); i++)
	{
		auto ntf = DirectX::XMLoadFloat4x4(&rig.nodes[i].transformation);

		if (!m_imgui_bindPose && i > 1)
				ntf = CalculateNodeTransformation(anim, i, m_currentTick);

		hereditaryTFs.push_back(ntf);

		/*DirectX::XMMATRIX imguiMatrix = DirectX::XMMatrixScaling(m_imgui_scale[i].x, m_imgui_scale[i].y, m_imgui_scale[i].z) *
			DirectX::XMMatrixRotationRollPitchYaw(m_imgui_rot[i].x, m_imgui_rot[i].y, m_imgui_rot[i].z) *
			DirectX::XMMatrixTranslation(m_imgui_pos[i].x, m_imgui_pos[i].y, m_imgui_pos[i].z);*/

			//hereditaryTFs.back() *= imguiMatrix;
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
};
void BoneJovi::GetBoned(DOG::ImportedAnimation ia)
{
	m_animations.push_back(ia);
	for (auto& om : ia.jointOffsets)
		m_vsJoints.push_back(om);
};

DirectX::XMVECTOR BoneJovi::GetAnimationComponent(const DOG::AnimationData& animation, KeyType component, i32 nodeID, f32 tick)
{
	using namespace DirectX;

	std::vector<DOG::AnimationKey> keys;
	if (component == KeyType::Scale)
		keys = animation.scaKeys.at(nodeID);
	else if (component == KeyType::Rotation)
		keys = animation.rotKeys.at(nodeID);
	else
		keys = animation.posKeys.at(nodeID);

	if (keys.size() == 1)
		return XMLoadFloat4(&keys[0].value);

	// Dirty
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