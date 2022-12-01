#include "MusicSystems.h"

using namespace DOG;

PlayMusicSystem::PlayMusicSystem()
{
	entity e = EntityManager::Get().CreateEntity();
	EntityManager::Get().AddComponent<MusicPlayer>(e);
	EntityManager::Get().AddComponent<AudioComponent>(e);

	m_ambienceSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsAmbience.wav");
	m_actionSong = AssetManager::Get().LoadAudio("Assets/Audio/Music/RogueRobotsAction.wav");

	m_timerForAmbientMusicToStart = 0.0f;
}

void PlayMusicSystem::OnUpdate(MusicPlayer&, DOG::AudioComponent& audio)
{
	bool foundAggro = false;
	EntityManager::Get().Collect<AgentAggroComponent>().Do([&](AgentAggroComponent) {
		foundAggro = true;
		audio.volume = 1.0f;
		});

	if (foundAggro && (!audio.playing || audio.assetID == m_ambienceSong))
	{
		audio.assetID = m_actionSong;
		audio.shouldPlay = true;
		audio.volume = 1.0f;
	}
	else if (audio.playing && audio.assetID == m_actionSong && !foundAggro)
	{
		audio.volume -= 0.15f * Time::DeltaTime();
		if (audio.volume <= 0.0f)
		{
			audio.volume = 0.0f;
			audio.shouldStop = true;
			playAmbience = false;
		}
	}
	else if (!audio.playing && m_timerForAmbientMusicToStart < Time::ElapsedTime())
	{
		if (audio.assetID == m_ambienceSong && !playAmbience)
		{
			m_timerForAmbientMusicToStart = Time::ElapsedTime() + WAIT_TIME_FOR_AMBIENT_MUSIC;
			playAmbience = true;
		}
		else
		{
			audio.assetID = m_ambienceSong;
			audio.shouldPlay = true;
			audio.volume = 1.0f;
			playAmbience = false;
		}
	}
	std::cout << "Timer: " << m_timerForAmbientMusicToStart << "\n";
	std::cout << "Elapsed Time: " << Time::ElapsedTime() << "\n";
}
