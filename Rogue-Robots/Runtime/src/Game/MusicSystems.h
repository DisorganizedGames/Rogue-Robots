#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class PlayMusicSystem : public DOG::ISystem
{
private:
	u32 m_ambienceSong;
	u32 m_actionSong;
	static constexpr f32 WAIT_TIME_FOR_AMBIENT_MUSIC = 90.0f;
	f32 m_timerForAmbientMusicToStart;
	bool playAmbience = true;

public:
	PlayMusicSystem();

	SYSTEM_CLASS(MusicPlayer, DOG::AudioComponent);
	ON_UPDATE(MusicPlayer, DOG::AudioComponent);

	void OnUpdate(MusicPlayer& musicPlayer, DOG::AudioComponent& audio);
};