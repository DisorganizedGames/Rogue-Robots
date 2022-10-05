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

	private:
		static inline std::unique_ptr<AudioDevice> s_device = nullptr;
		static inline bool s_deviceInitialized = false;
	};
}

