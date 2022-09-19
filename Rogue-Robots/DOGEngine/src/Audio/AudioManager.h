#pragma once
#include "AudioDevice.h"
#include "AudioPlayerComponent.h"

namespace DOG
{
	class AudioManager
	{
	public:
		AudioManager();
		~AudioManager() = default;
		
		AudioManager(AudioManager&& other) = delete;
		AudioManager(const AudioManager& other) = delete;
	public:
		void Play(AudioPlayerComponent& audioPlayerComponent);

		void Stop(AudioPlayerComponent& audioPlayerComponent);

		bool HasFinished(const AudioPlayerComponent& audioPlayerComponent) const;

	private:
		static inline constexpr const u32 MAX_SOURCES = 8;

		AudioDevice m_device;
		std::vector<std::unique_ptr<SourceVoice>> m_sources;
	
	private:
		u64 GetFreeVoice(const WAVProperties& wavProperties);
	};
}