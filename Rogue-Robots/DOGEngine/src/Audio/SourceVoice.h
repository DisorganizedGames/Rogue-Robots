#pragma once
#include "AudioFileReader.h"

namespace DOG
{
	struct SourceVoiceSettings
	{
		f32 volume = 1;
	};

	class SourceVoice
	{

	public:
		SourceVoice(IXAudio2SourceVoice* sourceVoice, const WAVProperties& properties, const SourceVoiceSettings& settings = {});
		SourceVoice(SourceVoice&& other) noexcept;
		~SourceVoice();

		// Delete copy contructor. 
		SourceVoice(const SourceVoice& other) = delete;

	public:
		// Plays a single buffer to the end. Does not support additional buffers
		void Play(const u8* buffer, size_t bufferSize);

		// Waits for all currently queued buffers to finish playing
		void WaitForEnd();

		// Returns true if all queued buffers have finished playing, false otherwise
		bool HasFinished();


	private:
		IXAudio2SourceVoice* sourceVoice = nullptr;
		WAVProperties audioProperties;
		SourceVoiceSettings voiceSettings;
	};	
}

