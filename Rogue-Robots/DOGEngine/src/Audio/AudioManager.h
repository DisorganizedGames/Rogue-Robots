#pragma once
#include "Audio.h"

namespace DOG
{
	class AudioManager
	{
	public:
		static void Initialize();
		static void Destroy();

		static void AudioSystem();

		static void StopAudioOnDeferredEntities();

		static void SetMasterVolume(f32 volume) noexcept { if (s_deviceInitialized) s_device->SetMasterVolume(volume); };

	private:
		static inline std::unique_ptr<AudioDevice> s_device = nullptr;
		static inline bool s_deviceInitialized = false;
	};
}

