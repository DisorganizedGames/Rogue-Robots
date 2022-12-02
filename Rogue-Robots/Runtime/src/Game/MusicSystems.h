#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class PlayMusicSystem : public DOG::ISystem
{
private:
	u32 m_ambienceSong;
	u32 m_actionSong;
	static constexpr f32 WAIT_TIME_FOR_AMBIENT_MUSIC = 65.0f;
	static constexpr f32 PLAY_TIME_FOR_AMBIENT_MUSIC = 160.0f;
	f32 m_timerForAmbientMusic;
	bool playAmbience = true;

	static constexpr f32 TIME_FOR_AFTER_AGGRO = 5.0f;
	f32 m_timerForActionToContinue;
	bool shouldPlayAction = false;

	DOG::entity m_ambienceMusicEntity;
	DOG::entity m_actionMusicEntity;

public:
	PlayMusicSystem();

	SYSTEM_CLASS(MusicPlayer);
	ON_UPDATE(MusicPlayer);

	void OnUpdate(MusicPlayer& musicPlayer);
};