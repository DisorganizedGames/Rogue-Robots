#pragma once
#include <vector>
//#include <Assimp/scene.h>           // Output data structure
#include <unordered_map>

#include "DOGEngineTypes.h"
//#include "Types/AssetTypes.h"
//#include "AssimpImporter.h"
//#include "ImGUI/imgui.h"
//#include "Types/GraphicsTypes.h"
//#include "AssetManager.h"


enum class KeyType
{
	Scale,
	Rotation,
	Translation,
};

class BoneJovi
{
private:
	// tmp
	struct LastSRT
	{
		u8 srt[3] = { 0, 0, 0 };
	};
	std::vector<LastSRT> m_lastSRTkeys;
	f32 m_currentTick = 0.0f;
private:
	
public:
	BoneJovi();
	~BoneJovi();
	void UpdateSkeleton(u32 skeletonId, u32 animId, f32 dt);
	void GetBoned(DOG::ImportedAnimation ia);
	i32 AddJob(i32 skeletonID, i32 animationID, f32 animationTime = 0.0f);
	std::vector<DirectX::XMFLOAT4X4>& GetBones() { return m_vsJoints; };

private:
	//DirectX::FXMMATRIX CalculateBlendTransformation(i32 nodeID);
	DirectX::FXMMATRIX CalculateNodeTransformation(const DOG::AnimationData&, i32 nodeID, f32 animTick);
	DirectX::XMVECTOR GetAnimationComponent(const DOG::AnimationData&, KeyType component, i32 nodeID, f32 tick);
	std::vector<DOG::ImportedAnimation> m_animations;
	std::vector<DirectX::XMFLOAT4X4> m_vsJoints;
private:
	// IMGUI RELATED
	bool m_imgui_testAnimationBlend = false;
	f32 m_imgui_blend = 0.0f;
	i32 m_imgui_animation2 = 0;
	f32 m_imgui_animTime2 = 0.0f;

	bool m_imgui_rootTranslation = true; // apply root animation translation or not
	bool m_imgui_playAnimation = true;
	bool m_imgui_bindPose = false;
	i32 m_imgui_selectedBone = 0;
	i32 m_imgui_animation = 0;
	f32 m_imgui_timeScale = 1.0f;
	f32 m_imgui_animTime = 0.0f;
	std::vector<DirectX::XMFLOAT3> m_imgui_scale;
	std::vector<DirectX::XMFLOAT3> m_imgui_rot;
	std::vector<DirectX::XMFLOAT3> m_imgui_pos;
public:
	//void SpawnControlWindow()
	//{
	//	if (ImGui::Begin("BonneJoints"))
	//	{
	//		ImGui::Columns(2, nullptr, true);
	//		/*if (ImGui::BeginCombo("animation1", m_animations[m_imgui_animation].name.c_str()))
	//		{
	//			for (i32 i = 0; i < std::size(m_animations); i++)
	//				if (ImGui::Selectable((m_animations[i].name + "  " + std::to_string(i)).c_str(), (i == m_imgui_animation)))
	//					m_imgui_animation = i;
	//			ImGui::EndCombo();
	//		}*/
	//		ImGui::SliderFloat("Animation Time", &m_imgui_animTime, 0.0f, 1.0f, "%.5f");
	//		ImGui::SliderFloat("Time Scale", &m_imgui_timeScale, -2.0f, 2.0f, "%.3f");
	//		ImGui::Checkbox("Play Animation", &m_imgui_playAnimation);
	//		ImGui::Checkbox("Bindpose", &m_imgui_bindPose);
	//		ImGui::Checkbox("RootTranslation", &m_imgui_rootTranslation);
	//		m_imgui_playAnimation = m_imgui_bindPose ? false : m_imgui_playAnimation;

	//		if (m_imgui_playAnimation)
	//			m_imgui_animTime2 = m_imgui_animTime;
	//		/*if (ImGui::BeginCombo("tfs", m_nodes[m_imgui_selectedBone].name.c_str()))
	//		{
	//			for (i32 i = 0; i < std::size(m_nodes); i++)
	//				if (ImGui::Selectable((m_nodes[i].name + "  " + std::to_string(i)).c_str(), (i == m_imgui_selectedBone)))
	//					m_imgui_selectedBone = i;
	//			ImGui::EndCombo();
	//		}*/
	//		/*ImGui::Text("Orientation");
	//		ImGui::SliderAngle("Roll", &m_imgui_rot[m_imgui_selectedBone].x, -180.0f, 180.0f);
	//		ImGui::SliderAngle("Pitch", &m_imgui_rot[m_imgui_selectedBone].y, -180.0f, 180.0f);
	//		ImGui::SliderAngle("Yaw", &m_imgui_rot[m_imgui_selectedBone].z, -180.0f, 180.0f);

	//		ImGui::Text("Translation");
	//		ImGui::SliderFloat("pos X", &m_imgui_pos[m_imgui_selectedBone].x, -1.0f, 1.0f, "%.3f");
	//		ImGui::SliderFloat("pos Y", &m_imgui_pos[m_imgui_selectedBone].y, -1.0f, 1.0f, "%.3f");
	//		ImGui::SliderFloat("pos Z", &m_imgui_pos[m_imgui_selectedBone].z, -1.0f, 1.0f, "%.3f");

	//		ImGui::Text("Scale");
	//		ImGui::SliderFloat("X", &m_imgui_scale[m_imgui_selectedBone].x, -10.0f, 10.0f, "%.1f");
	//		ImGui::SliderFloat("Y", &m_imgui_scale[m_imgui_selectedBone].z, -10.0f, 10.0f, "%.1f");
	//		ImGui::SliderFloat("Z", &m_imgui_scale[m_imgui_selectedBone].y, -10.0f, 10.0f, "%.1f");
	//		ImGui::NextColumn();
	//		if (ImGui::BeginCombo("animation2", m_animations[m_imgui_animation2].name.c_str()))
	//		{
	//			for (i32 j = 0; j < std::size(m_animations); j++)
	//				if (ImGui::Selectable((m_animations[j].name + "  " + std::to_string(j)).c_str(), (j == m_imgui_animation2)))
	//					m_imgui_animation2 = j;
	//			ImGui::EndCombo();
	//		}*/
	//		ImGui::SliderFloat("Animation Time2", &m_imgui_animTime2, 0.0f, 1.0f, "%.5f");
	//		ImGui::SliderFloat("blendFactor", &m_imgui_blend, 0.0f, 1.0f, "%.5f");
	//		ImGui::Checkbox("Blend", &m_imgui_testAnimationBlend);
	//	}
	//}
};

