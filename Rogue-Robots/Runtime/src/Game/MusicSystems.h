#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class PlayMusicSystem : public DOG::ISystem
{
private:
	u32 m_mainMenuSong;
	u32 m_ambienceSong;
	u32 m_actionSong;
	static constexpr f32 WAIT_TIME_FOR_AMBIENT_MUSIC = 30.0f;
	static constexpr f32 PLAY_TIME_FOR_AMBIENT_MUSIC = 160.0f;
	f32 m_timerForAmbientMusic;
	bool playAmbience = true;
	bool playMainMenu = true;

	static constexpr f32 TIME_FOR_AFTER_AGGRO = 5.0f;
	f32 m_timerForActionToContinue;
	bool shouldPlayAction = false;

	static constexpr f32 MAINMENU_MUSIC_VOLUME = 0.5f;
	static constexpr f32 AMBIENT_MUSIC_VOLUME = 0.5f;
	static constexpr f32 ACTION_MUSIC_VOLUME = 0.5f;

	DOG::entity m_mainMenuMusicEntity;
	DOG::entity m_ambienceMusicEntity;
	DOG::entity m_actionMusicEntity;

	bool m_inMainMenuLastFrame = true;
public:
	PlayMusicSystem();

	SYSTEM_CLASS(MusicPlayer);
	ON_UPDATE(MusicPlayer);

	void OnUpdate(MusicPlayer& musicPlayer);
};


class AmbientSoundSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(AmbientSoundComponent, DOG::AudioComponent);
	ON_UPDATE(AmbientSoundComponent, DOG::AudioComponent);
	void OnUpdate(AmbientSoundComponent& ambientSound, DOG::AudioComponent& audioPlayer);
};