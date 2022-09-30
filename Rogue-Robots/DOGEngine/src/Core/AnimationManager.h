#pragma once

#include "../ECS/Component.h"
#include "DOGEngineTypes.h"
#include "../Graphics/RHI/ImGUIBackend.h"


// rip BoneJovi
class AnimationManager 
{
private:
	static constexpr u8 MAX_ANIMATIONS = 2;
	enum class KeyType
	{
		Scale,
		Rotation,
		Translation,
	};
	// tmp, unused for now
	struct LastSRT
	{
		u8 srt[3] = { 0, 0, 0 };
	};
	std::vector<LastSRT> m_lastSRTkeys;
public:
	AnimationManager();
	~AnimationManager();
	void UpdateSkeleton(const DOG::AnimationComponent& animator, const DOG::ImportedAnimation& rig);
	void UpdateSkeleton(u32 skeletonId, f32 dt);
	void SetImportedAnimations(DOG::ImportedAnimation& ia);
	void UpdateAnimationComponent(DOG::AnimationComponent& ac, const std::vector<DOG::AnimationData>& animations, const f32 dt) const;
	std::vector<DirectX::XMFLOAT4X4>& GetBones() { return m_vsJoints; };
private:
	DirectX::FXMMATRIX CalculateBlendTransformation(i32 nodeID);
	DirectX::FXMMATRIX CalculateBlendTransformation(i32 nodeID, const DOG::ImportedAnimation& rig, const DOG::AnimationComponent& ac);
	DirectX::FXMMATRIX CalculateNodeTransformation(const DOG::AnimationData&, i32 nodeID, f32 animTick);
	DirectX::XMVECTOR GetAnimationComponent(const std::vector<DOG::AnimationKey>& keys, KeyType component, f32 tick);
	
	// Collection of the rigs and corresponding animations used in game (tmp)
	std::vector<DOG::ImportedAnimation> m_rigs;
	// Matrices to upload to Vertex Shader
	std::vector<DirectX::XMFLOAT4X4> m_vsJoints;
private:
	f32 m_currentTick = 0.0f;
	// IMGUI RELATED
	int m_imguiProfilePerformUpdate = 1;
	bool m_imguiTestAnimationBlend = false;
	f32 m_imguiBlend = 0.0f;
	i32 m_imguiAnimation2 = 0;
	f32 m_imguiAnimTime2 = 0.0f;
	bool m_imguiRootTranslation = false; // apply root animation translation or not
	bool m_imguiPlayAnimation = true;
	bool m_imguiBindPose = false;
	i32 m_imguiSelectedBone = 1;
	i32 m_imguiAnimation = 0;
	f32 m_imguiTimeScale = 1.0f;
	f32 m_imguiAnimTime = 0.0f;
	std::vector<DirectX::XMFLOAT3> m_imguiScale;
	std::vector<DirectX::XMFLOAT3> m_imguiRot;
	std::vector<DirectX::XMFLOAT3> m_imguiPos;
public:
	void SpawnControlWindow();
};

