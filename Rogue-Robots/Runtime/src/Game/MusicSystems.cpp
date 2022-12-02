#include "MusicSystems.h"

using namespace DOG;

PlayMusicSystem::PlayMusicSystem()
{
	m_ambienceSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsAmbience.wav");
	m_actionSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsAction.wav");

	entity e = EntityManager::Get().CreateEntity();
	EntityManager::Get().AddComponent<MusicPlayer>(e);

	m_ambienceMusicEntity = EntityManager::Get().CreateEntity();
	AudioComponent& ambienceAudioComponent = EntityManager::Get().AddComponent<AudioComponent>(m_ambienceMusicEntity);
	ambienceAudioComponent.assetID = m_ambienceSong;
	ambienceAudioComponent.shouldPlay = true;
	ambienceAudioComponent.volume = AMBIENT_MUSIC_VOLUME;
	ambienceAudioComponent.loopStart = 0.0f;
	ambienceAudioComponent.loopEnd = -1.0f;
	ambienceAudioComponent.loop = true;

	m_actionMusicEntity = EntityManager::Get().CreateEntity();
	AudioComponent& actionAudioComponent = EntityManager::Get().AddComponent<AudioComponent>(m_actionMusicEntity);
	actionAudioComponent.assetID = m_actionSong;
	actionAudioComponent.shouldPlay = true;
	actionAudioComponent.volume = 0.0f;
	actionAudioComponent.loopStart = 0.0f;
	actionAudioComponent.loopEnd = -1.0f;
	actionAudioComponent.loop = true;

	m_timerForAmbientMusic = 0.0f;

	playAmbience = true;
	shouldPlayAction = false;
}

void PlayMusicSystem::OnUpdate(MusicPlayer& musicPlayer)
{
	AudioComponent& actionAudioComponent = EntityManager::Get().GetComponent<AudioComponent>(m_actionMusicEntity);
	AudioComponent& ambienceAudioComponent = EntityManager::Get().GetComponent<AudioComponent>(m_ambienceMusicEntity);

	bool foundAggro = false;
	EntityManager::Get().Collect<AgentAggroComponent>().Do([&](AgentAggroComponent) {
		foundAggro = true;
		actionAudioComponent.volume = ACTION_MUSIC_VOLUME;
		ambienceAudioComponent.volume = 0.0f;
		m_timerForActionToContinue = -1.0f;
		});

	if (musicPlayer.inMainMenu)
	{
		playAmbience = true;
		m_timerForAmbientMusic = 0.0f;
		actionAudioComponent.volume = 0.0f;
		ambienceAudioComponent.volume = AMBIENT_MUSIC_VOLUME;
		musicPlayer.inMainMenu = false;
		shouldPlayAction = false;
		return;
	}

	if (foundAggro)
	{
		actionAudioComponent.volume = ACTION_MUSIC_VOLUME;
		ambienceAudioComponent.volume = 0.0f;

		m_timerForActionToContinue = -1.0f;

		shouldPlayAction = true;
	}
	else if (!foundAggro && m_timerForActionToContinue <= (f32)Time::ElapsedTime() && shouldPlayAction)
	{
		if (m_timerForActionToContinue < 0.0f)
		{
			m_timerForActionToContinue = TIME_FOR_AFTER_AGGRO + (f32)Time::ElapsedTime();
			return;
		}

		actionAudioComponent.volume -= 0.2f * (f32)Time::DeltaTime();
		if (actionAudioComponent.volume <= 0.0f)
		{
			actionAudioComponent.volume = 0.0f;
			m_timerForAmbientMusic = -1.0f;
			playAmbience = false;
			shouldPlayAction = false;
		}
	}
	else if (m_timerForAmbientMusic <= (f32)Time::ElapsedTime())
	{
		ambienceAudioComponent.volume = 0.0f;
		if (!playAmbience)
		{
			m_timerForAmbientMusic = (f32)Time::ElapsedTime() + WAIT_TIME_FOR_AMBIENT_MUSIC;
			playAmbience = true;
		}
		else
		{
			m_timerForAmbientMusic = (f32)Time::ElapsedTime() + PLAY_TIME_FOR_AMBIENT_MUSIC;
			playAmbience = false;
		}
	}
	else if (!shouldPlayAction)
	{
		const float lerpValue = 5.0f;

		if (!playAmbience && m_timerForAmbientMusic - (f32)Time::ElapsedTime() <= lerpValue)
		{
			ambienceAudioComponent.volume = AMBIENT_MUSIC_VOLUME * (m_timerForAmbientMusic - (f32)Time::ElapsedTime()) / lerpValue;
		}
		else if (!playAmbience && m_timerForAmbientMusic - (f32)Time::ElapsedTime() > PLAY_TIME_FOR_AMBIENT_MUSIC - lerpValue)
		{
			ambienceAudioComponent.volume = AMBIENT_MUSIC_VOLUME * (1.0f - (m_timerForAmbientMusic - (f32)Time::ElapsedTime() - (PLAY_TIME_FOR_AMBIENT_MUSIC - lerpValue)) / lerpValue);
		}
		else if (!playAmbience)
		{
			ambienceAudioComponent.volume = AMBIENT_MUSIC_VOLUME;
		}
		else
		{
			ambienceAudioComponent.volume = 0.0f;
		}
	}
}
