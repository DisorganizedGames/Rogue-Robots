#pragma once
#include "AudioFileReader.h"
#include "SourceVoiceCallback.h"

namespace DOG
{
	struct SourceVoiceSettings
	{
		f32 volume = 1;
	};

	class SourceVoice
	{

	public:
		SourceVoice() = default;
		SourceVoice(IXAudio2SourceVoice* sourceVoice, const WAVProperties& properties, 
			std::unique_ptr<SourceVoiceCallback> callback, const SourceVoiceSettings& settings = {});
		SourceVoice(SourceVoice&& other) noexcept;
		~SourceVoice();

		// Delete copy contructor. 
		SourceVoice(const SourceVoice& other) = delete;
		
		SourceVoice& operator =(SourceVoice&& other) noexcept;
	public:
		// Plays a single buffer to the end. Does not support additional buffers
		void Play(std::vector<u8>&& buffer);
		
		void Stop();

		// Waits for all currently queued buffers to finish playing
		void WaitForEnd();

		// Returns true if all queued buffers have finished playing, false otherwise
		bool HasFinished();
		
		// Set new settings for the source voice
		void SetSettings(const SourceVoiceSettings& settings);

		const WAVProperties& GetWAVProperties() const;


	private:
		IXAudio2SourceVoice* m_sourceVoice = nullptr;
		WAVProperties m_audioProperties;
		SourceVoiceSettings m_voiceSettings;
		std::vector<std::vector<u8>> m_buffers;

		std::unique_ptr<SourceVoiceCallback> m_callback;
	};	
}

