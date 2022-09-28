#pragma once

#include "DOGEngineTypes.h"
#include "../Graphics/RHI/ImGUIBackend.h"


enum class KeyType
{
	Scale,
	Rotation,
	Translation,
};

// rip BoneJovi
class AnimationManager 
{
private:
	// tmp, unused for now
	struct LastSRT
	{
		u8 srt[3] = { 0, 0, 0 };
	};
	std::vector<LastSRT> m_lastSRTkeys;
	f32 m_currentTick = 0.0f;
public:
	AnimationManager();
	~AnimationManager();
	void UpdateSkeleton(u32 skeletonId, f32 dt);
	void SetImportedAnimations(DOG::ImportedAnimation& ia);
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
	int m_imguiprofilePerformUpdate = 1;
	bool m_imguitestAnimationBlend = false;
	f32 m_imguiblend = 0.0f;
	i32 m_imguianimation2 = 0;
	f32 m_imguianimTime2 = 0.0f;
	bool m_imguirootTranslation = true; // apply root animation translation or not
	bool m_imguiplayAnimation = true;
	bool m_imguibindPose = false;
	i32 m_imguiselectedBone = 1;
	i32 m_imguianimation = 0;
	f32 m_imguitimeScale = 1.0f;
	f32 m_imguianimTime = 0.0f;
	std::vector<DirectX::XMFLOAT3> m_imguiscale;
	std::vector<DirectX::XMFLOAT3> m_imguirot;
	std::vector<DirectX::XMFLOAT3> m_imguipos;
public:
	void SpawnControlWindow();
};

