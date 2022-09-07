#pragma once
#include "SourceVoice.h"

namespace DOG
{
	class AudioDevice
	{
	public:
		AudioDevice();
		~AudioDevice();

	private:
		IXAudio2* xAudio = nullptr;
		IXAudio2MasteringVoice* masteringVoice = nullptr;

	public:
		SourceVoice CreateSourceVoice(const WAVProperties& options, const SourceVoiceSettings& settings = {});
	};
}