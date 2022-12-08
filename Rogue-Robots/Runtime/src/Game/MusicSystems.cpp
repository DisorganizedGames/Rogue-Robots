#include "MusicSystems.h"

using namespace DOG;

PlayMusicSystem::PlayMusicSystem()
{
	m_mainMenuSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsMainMenu.wav");
	m_ambienceSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsAmbience.wav");
	m_actionSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsAction.wav");

	entity e = EntityManager::Get().CreateEntity();
	EntityManager::Get().AddComponent<MusicPlayer>(e);

	m_mainMenuMusicEntity = EntityManager::Get().CreateEntity();
	AudioComponent& mainMenuAudioComponent = EntityManager::Get().AddComponent<AudioComponent>(m_mainMenuMusicEntity);
	mainMenuAudioComponent.assetID = m_mainMenuSong;
	mainMenuAudioComponent.shouldPlay = true;
	mainMenuAudioComponent.volume = MAINMENU_MUSIC_VOLUME;
	mainMenuAudioComponent.loopStart = 0.0f;
	mainMenuAudioComponent.loopEnd = -1.0f;
	mainMenuAudioComponent.loop = true;

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

	playMainMenu = true;
	playAmbience = false;
	shouldPlayAction = false;
}

void PlayMusicSystem::OnUpdate(MusicPlayer& musicPlayer)
{
	AudioComponent& mainMenuAudioComponent = EntityManager::Get().GetComponent<AudioComponent>(m_mainMenuMusicEntity);
	AudioComponent& actionAudioComponent = EntityManager::Get().GetComponent<AudioComponent>(m_actionMusicEntity);
	AudioComponent& ambienceAudioComponent = EntityManager::Get().GetComponent<AudioComponent>(m_ambienceMusicEntity);

	bool foundAggro = false;
	EntityManager::Get().Collect<AgentAggroComponent>().Do([&](AgentAggroComponent) {
		foundAggro = true;
		actionAudioComponent.volume = ACTION_MUSIC_VOLUME;
		ambienceAudioComponent.volume = 0.0f;
		m_timerForActionToContinue = -1.0f;
		});

	if (!m_inMainMenuLastFrame && musicPlayer.inMainMenu) //If we were not in main menu last frame but are now. It means we were ingame and went back to main menu.
	{
		playMainMenu = true;
		m_timerForAmbientMusic = 0.0f;
		mainMenuAudioComponent.shouldStop = true;
		mainMenuAudioComponent.volume = MAINMENU_MUSIC_VOLUME;
		actionAudioComponent.volume = 0.0f;
		ambienceAudioComponent.volume = 0.0f;

		musicPlayer.inMainMenu = false;
		shouldPlayAction = false;
		m_inMainMenuLastFrame = true;
	}
	else if (musicPlayer.inMainMenu)
	{
		playMainMenu = true;
		m_timerForAmbientMusic = 0.0f;
		if (!mainMenuAudioComponent.playing)
		{
			mainMenuAudioComponent.shouldPlay = true;
		}
		mainMenuAudioComponent.volume = MAINMENU_MUSIC_VOLUME;
		actionAudioComponent.volume = 0.0f;
		ambienceAudioComponent.volume = 0.0f;
		
		musicPlayer.inMainMenu = false;
		shouldPlayAction = false;
		m_inMainMenuLastFrame = true;
		return;
	}
	else
	{
		mainMenuAudioComponent.volume = 0.0f;
		m_inMainMenuLastFrame = false;
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

void AmbientSoundSystem::OnUpdate(AmbientSoundComponent& ambientSound, DOG::AudioComponent& audioPlayer)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	if (ambientSound.startup || (audioPlayer.playing && ambientSound.repeatTime < 0))
	{
		std::normal_distribution<float> nd(ambientSound.meanRepeatTime, ambientSound.stdDiv);
		ambientSound.repeatTime = nd(gen) + ambientSound.singleTimeStartOffsetTime;
		ambientSound.singleTimeStartOffsetTime = 0;
		ambientSound.startup = false;
	}
	else if (!audioPlayer.playing && ambientSound.repeatTime >= 0)
	{
		ambientSound.repeatTime -= Time::DeltaTime<TimeType::Seconds, f32>();
	}
	else if (ambientSound.repeatTime < 0)
	{
		audioPlayer.shouldPlay = true;
	}
}
