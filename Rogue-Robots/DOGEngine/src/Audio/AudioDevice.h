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
		IXAudio2* m_xAudio = nullptr;
		IXAudio2MasteringVoice* m_masteringVoice = nullptr;

	public:
		SourceVoice CreateSourceVoice(const WAVProperties& options, const SourceVoiceSettings& settings = {});
	};
}