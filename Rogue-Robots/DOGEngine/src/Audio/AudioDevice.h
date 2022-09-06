#pragma once
#include <xaudio2.h>
#include <x3daudio.h>

namespace DOG
{
	class AudioDevice
	{
	public:
		AudioDevice();
		~AudioDevice();

	private:
		IXAudio2* xAudio = nullptr;
		IXAudio2MasteringVoice *masteringVoice = nullptr;
	};
}