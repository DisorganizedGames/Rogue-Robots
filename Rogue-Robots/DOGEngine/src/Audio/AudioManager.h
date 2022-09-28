#pragma once
#include "AudioDevice.h"
#include "AudioPlayerComponent.h"

namespace DOG
{
	class AudioManager
	{
	public:
		AudioManager() = delete;
		//~AudioManager() = default;
		
		//AudioManager(AudioManager&& other) = delete;
		//AudioManager(const AudioManager& other) = delete;
	public:
		static void Initialize();
		static void Destroy();

		static void Play(AudioPlayerComponent& audioPlayerComponent);

		static void Stop(AudioPlayerComponent& audioPlayerComponent);

		static bool HasFinished(const AudioPlayerComponent& audioPlayerComponent);

	private:
		static inline constexpr const u32 MAX_SOURCES = 8;

		static inline AudioDevice* s_device = nullptr;
		static inline auto s_sources = std::vector<std::unique_ptr<SourceVoice>>(MAX_SOURCES);
	
	private:
		static u64 GetFreeVoice(const WAVProperties& wavProperties);
	};
}