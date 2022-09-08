#include "AudioManager.h"

using namespace DOG;

AudioManager::AudioManager()
{
	m_sources.resize(MAX_SOURCES);
}

void AudioManager::Play(AudioPlayerComponent& audioPlayerComponent)
{
	auto& [audioID, volume, _] = audioPlayerComponent;
	
	// TODO: HIGHLY TEMPORARY
	auto [wavProp, data] = ReadWAV("example.wav");

	u64 freeVoiceIndex = GetFreeVoice(wavProp);

	SourceVoiceSettings settings = {
		.volume = volume,
	};
	auto& source = m_sources[freeVoiceIndex];

	source->SetSettings(settings);
	
	audioPlayerComponent.voiceID = freeVoiceIndex;
	source->Play(std::move(data));
}

void AudioManager::Stop(AudioPlayerComponent& audioPlayerComponent)
{
	u64 voiceID = audioPlayerComponent.voiceID;
	auto& voice = m_sources[voiceID];
	if (!voice->HasFinished())
	{
		voice->Stop();
	}
	audioPlayerComponent.voiceID = -1;
}

void AudioManager::WaitForEnd(AudioPlayerComponent& audioPlayerComponent)
{
	i64 voiceID = audioPlayerComponent.voiceID;
	if (audioPlayerComponent.voiceID < 0 || audioPlayerComponent.voiceID > MAX_SOURCES-1)
	{
		return;
	}

	m_sources[voiceID]->WaitForEnd();
}

u64 AudioManager::GetFreeVoice(const WAVProperties& wavProperties)
{
	u64 index = 0;
	// Find compatible voice
	for (auto& source: m_sources)
	{
		if (!source)
			continue;

		if (!source->HasFinished())
			continue;

		auto sourceWAVProperties = source->GetWAVProperties();
		if (memcmp(&wavProperties, &sourceWAVProperties, sizeof(WAVProperties)) == 0)
		{
			return index;
		}
		++index;
	}
	index = 0;

	// Find empty voice and create a new one with the provided properties
	for (auto& source: m_sources)
	{
		if (!source)
		{
			source = std::make_unique<SourceVoice>();
			*source = m_device.CreateSourceVoice(wavProperties);
			return index;
		}
		++index;
	}
	index = 0;

	// Free unused voice(s?) and use the freed voice's place
	for (auto& source: m_sources)
	{
		if (!source)
			continue; // This really shouldn't happen since we just saw there were no free spots

		if (source->HasFinished())
		{
			source.reset();
			return index;
		}
		++index;
	}

	// TODO: NoVoiceAvailableError
	throw std::runtime_error("No available voice found");
}

