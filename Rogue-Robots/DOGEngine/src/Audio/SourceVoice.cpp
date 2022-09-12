#include "SourceVoice.h"

using namespace DOG;

SourceVoice::SourceVoice(IXAudio2SourceVoice* sourceVoice, const WAVProperties& properties,
	std::unique_ptr<SourceVoiceCallback> callback, const SourceVoiceSettings& settings)

	: m_sourceVoice(sourceVoice), m_audioProperties(properties), m_callback(std::move(callback))
{
	SetSettings(settings);
}

SourceVoice::SourceVoice(SourceVoice&& other) noexcept
	: m_sourceVoice(other.m_sourceVoice), m_audioProperties(other.m_audioProperties),
	m_voiceSettings(other.m_voiceSettings), m_callback(std::move(other.m_callback))
{
	other.m_sourceVoice = nullptr;
}

SourceVoice::~SourceVoice()
{
	if (m_sourceVoice)
		m_sourceVoice->DestroyVoice();
}

SourceVoice& SourceVoice::operator=(SourceVoice&& other) noexcept
{
	m_sourceVoice = other.m_sourceVoice;
	m_audioProperties = other.m_audioProperties;
	m_voiceSettings = other.m_voiceSettings;

	m_callback = std::move(other.m_callback);

	other.m_sourceVoice = nullptr;
	return *this;
}

void SourceVoice::Play(std::vector<u8>&& buffer)
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

	// TODO: How the fuck do we store this buffer
	m_buffers.push_back(std::move(buffer));
}

void SourceVoice::Stop()
{
	HR hr = m_sourceVoice->Stop();
	hr.try_fail("Failed to stop Source Voice");

	hr = m_sourceVoice->FlushSourceBuffers();
	hr.try_fail("Failed to flush source voice queued buffers");
}

void SourceVoice::WaitForEnd()
{
	XAUDIO2_VOICE_STATE state;
	m_sourceVoice->GetState(&state);
	if (state.BuffersQueued > 0)
	{
		m_callback->WaitForStreamEnd();
	}
	m_buffers.clear();
	return;
}

bool SourceVoice::HasFinished()
{
	XAUDIO2_VOICE_STATE state;
	m_sourceVoice->GetState(&state);

	return state.BuffersQueued == 0;
}

void SourceVoice::SetSettings(const SourceVoiceSettings& settings)
{
	this->m_voiceSettings = settings;
	m_sourceVoice->SetVolume(settings.volume);
}

const WAVProperties& SourceVoice::GetWAVProperties() const
{
	return m_audioProperties;
}

