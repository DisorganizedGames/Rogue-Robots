#pragma once
#include <vector>
//#include <Assimp/scene.h>           // Output data structure
#include <unordered_map>

#include "DOGEngineTypes.h"
//#include "Types/AssetTypes.h"
//#include "AssimpImporter.h"
#include "../Graphics/RHI/ImGUIBackend.h"
//#include "../Graphics/RenderBackend_DX12.h"
//#include "../Graphics/ImGUIBackend_DX12.h"
//#include "../Graphics/RenderDevice_DX12.h"
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
	// tmp, unused for now
	struct LastSRT
	{
		u8 srt[3] = { 0, 0, 0 };
	};
	std::vector<LastSRT> m_lastSRTkeys;
	f32 m_currentTick = 0.0f;
	struct AnimationJob
	{
		u32 rigID;
		u32 animationID;
		//f32 timeScale;
		//f32 CurrentTick;
		
		// delay/loop/interupt/append/culled etc. ?
	};
private:
	
public:
	BoneJovi();
	~BoneJovi();
	void UpdateSkeleton(u32 skeletonId, f32 dt);
	void SetJoints(DOG::ImportedAnimation& ia);
	//i32 AddJob(i32 skeletonID, i32 animationID, f32 animationTime = 0.0f);
	std::vector<DirectX::XMFLOAT4X4>& GetBones() { return m_vsJoints; };

private:
	DirectX::FXMMATRIX CalculateBlendTransformation(i32 nodeID);
	DirectX::FXMMATRIX CalculateNodeTransformation(const DOG::AnimationData&, i32 nodeID, f32 animTick);
	DirectX::XMVECTOR GetAnimationComponent(const std::vector<DOG::AnimationKey>& keys, KeyType component, f32 tick);
	
	// Collection of the rigs and corresponding animations used in game
	std::vector<DOG::ImportedAnimation> m_rigs; 
	// Matrices to upload to Vertex Shader
	std::vector<DirectX::XMFLOAT4X4> m_vsJoints;
private:
	// IMGUI RELATED
	int m_imgui_profilePerformUpdate = 1;
	bool m_imgui_testAnimationBlend = false;
	f32 m_imgui_blend = 0.0f;
	i32 m_imgui_animation2 = 0;
	f32 m_imgui_animTime2 = 0.0f;

	bool m_imgui_rootTranslation = true; // apply root animation translation or not
	bool m_imgui_playAnimation = true;
	bool m_imgui_bindPose = false;
	i32 m_imgui_selectedBone = 1;
	i32 m_imgui_animation = 0;
	f32 m_imgui_timeScale = 1.0f;
	f32 m_imgui_animTime = 0.0f;
	std::vector<DirectX::XMFLOAT3> m_imgui_scale;
	std::vector<DirectX::XMFLOAT3> m_imgui_rot;
	std::vector<DirectX::XMFLOAT3> m_imgui_pos;
public:
	void SpawnControlWindow();
};

