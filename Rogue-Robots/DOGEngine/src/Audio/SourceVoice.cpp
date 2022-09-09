#include "SourceVoice.h"

DOG::SourceVoice::SourceVoice(IXAudio2SourceVoice* sourceVoice, const WAVProperties& properties,
	std::unique_ptr<SourceVoiceCallback> callback, const SourceVoiceSettings& settings)

	: m_sourceVoice(sourceVoice), m_audioProperties(properties), m_callback(std::move(callback))
{
	m_voiceSettings = settings;
	sourceVoice->SetVolume(settings.volume);
}

DOG::SourceVoice::SourceVoice(SourceVoice&& other) noexcept
	: m_sourceVoice(other.m_sourceVoice), m_audioProperties(other.m_audioProperties), m_voiceSettings(other.m_voiceSettings)
{
	other.m_sourceVoice = nullptr;
}

DOG::SourceVoice::~SourceVoice()
{
	if (m_sourceVoice)
		m_sourceVoice->DestroyVoice();
}

void DOG::SourceVoice::Play(std::vector<u8>&& buffer)
{
	XAUDIO2_BUFFER xAudioBuffer = {
		.Flags = XAUDIO2_END_OF_STREAM,
		.AudioBytes = static_cast<u32>(buffer.size()),
		.pAudioData = buffer.data(),
		.PlayBegin = 0,
		.PlayLength = 0,
		.LoopBegin = 0,
		.LoopLength = 0,
		.LoopCount = 0,
		.pContext = nullptr,
	};

	HR hr = m_sourceVoice->SubmitSourceBuffer(&xAudioBuffer);
	hr.try_fail("Failed to queue XAudio Buffer");

	hr = m_sourceVoice->Start();
	hr.try_fail("Failed to start playing queued XAudio Buffer");
}

void DOG::SourceVoice::WaitForEnd()
{
	XAUDIO2_VOICE_STATE state;
	m_sourceVoice->GetState(&state);
	if (state.BuffersQueued > 0)
	{
		m_callback->WaitForStreamEnd();
	}
	return;
}

bool DOG::SourceVoice::HasFinished()
{
	XAUDIO2_VOICE_STATE state;
	m_sourceVoice->GetState(&state);

	return state.BuffersQueued == 0;
}

