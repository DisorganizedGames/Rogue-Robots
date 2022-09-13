#include "AudioManager.h"
#include <filesystem>
#include <vector>
#include "AudioFileReader.h"

using namespace DOG;

AudioManager::AudioManager()
{
	m_sources.resize(MAX_SOURCES);
}

void AudioManager::Play(AudioPlayerComponent& audioPlayerComponent)
{
	auto& [audioID, volume, _] = audioPlayerComponent;
	
	auto fileSize = std::filesystem::file_size("chilling.wav");

	// TODO: HIGHLY TEMPORARY
	WAVProperties wavProp;
	std::vector<u8> data;
	
	WAVFileReader wfr("chilling.wav");
	wavProp = wfr.ReadProperties();
	
	if (fileSize <= 4096)
	{
		auto [p, d] = ReadWAV("chilling.wav");
		data = std::move(d);
	}

	u64 freeVoiceIndex = GetFreeVoice(wavProp);

	SourceVoiceSettings settings = {
		.volume = volume,
	};
	auto& source = m_sources[freeVoiceIndex];

	source->SetSettings(settings);
	audioPlayerComponent.voiceID = freeVoiceIndex;
	
	if (fileSize > ((u64)18<<1))
	{
		source->PlayAsync(std::move(wfr));
	}
	else
	{
		source->Play(std::move(data));
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

