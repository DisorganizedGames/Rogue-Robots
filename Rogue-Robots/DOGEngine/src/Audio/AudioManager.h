#pragma once
#include "AudioDevice.h"
#include "AudioPlayerComponent.h"

namespace DOG
{
	class AudioManager
	{
	public:
		AudioManager() = delete;

	public:
		static void Initialize();
		static void Destroy();

		static void Play(AudioPlayerComponent& audioPlayerComponent);

		static void Stop(AudioPlayerComponent& audioPlayerComponent);

		static bool HasFinished(const AudioPlayerComponent& audioPlayerComponent);

	private:
		static inline constexpr const u32 MAX_SOURCES = 128;

		static inline std::unique_ptr<AudioDevice> s_device;
		static inline auto s_sources = std::vector<std::unique_ptr<SourceVoice>>(MAX_SOURCES);
		static inline bool s_audio_device_instantiated = false;
	
	private:
		static u64 GetFreeVoice(const WAVProperties& wavProperties);
	};
}