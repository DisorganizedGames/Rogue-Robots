#include "AudioManager.h"
#include <filesystem>
#include <vector>
#include "AudioFileReader.h"
#include "../Core/AssetManager.h"

using namespace DOG;

AudioManager::AudioManager()
{
	m_sources.resize(MAX_SOURCES);
}

void AudioManager::Play(AudioPlayerComponent& audioPlayerComponent)
{
	auto& [audioID, volume, _] = audioPlayerComponent;
	
	AudioAsset* asset = (AudioAsset*)AssetManager::Get().GetAsset(audioPlayerComponent.audioID);

	u64 freeVoiceIndex = GetFreeVoice(asset->properties);

	SourceVoiceSettings settings = {
		.volume = volume,
	};
	auto& source = m_sources[freeVoiceIndex];

	source->SetSettings(settings);
	audioPlayerComponent.voiceID = freeVoiceIndex;
	
	if (asset->async)
	{
		WAVFileReader wfr(asset->filePath);
		source->PlayAsync(std::move(wfr));
	}
	else
	{
		source->Play(asset->audioData);
	}
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
	// Find compatible voice
	for (int i = 0; i < m_sources.size(); ++i)
	{
		auto& source = m_sources[i];
		if (!source)
			continue;
		if (!source->HasFinished())
			continue;
		
		auto sourceWAVProperties = source->GetWAVProperties();
		if (memcmp(&wavProperties, &sourceWAVProperties, sizeof(WAVProperties)) == 0)
		{
			return i;
		}
	}

	// Find empty voice and create a new one with the provided properties
	for (int i = 0; i < m_sources.size(); ++i)
	{
		auto& source = m_sources[i];
		if (!source)
		{
			source = std::make_unique<SourceVoice>();
			*source = m_device.CreateSourceVoice(wavProperties);
			return i;
		}
	}

	// Free unused voice(s?) and use the freed voice's place
	for (int i = 0; i < m_sources.size(); ++i)
	{
		auto& source = m_sources[i];
		if (!source)
			continue;
		
		if (source->HasFinished())
		{
			source.reset();
			return i;
		}
	}

	throw NoVoiceAvailableError(); // TODO: Figure out a way to solve this... Software Mixing? More voices?
}

