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
	{
		m_playingAsync = false;
		Stop();
		m_callback->TriggerEnd();
		m_sourceVoice->DestroyVoice();
	}
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

void SourceVoice::Play(const std::vector<u8>& buffer)
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

void SourceVoice::PlayAsync(WAVFileReader&& fileReader)
{
	if (m_playingAsync)
		assert(false);

	constexpr u64 chunkSize = 16384;
	m_playingAsync = true;
	m_shouldStop = false;

	m_audioThread = std::jthread([&](WAVFileReader&& wfr)
		{
			u8 index = 0;
			std::array<std::vector<u8>, 2> buffers;

			buffers[index] = wfr.ReadNextChunk(chunkSize);
			bool finalBuffer = (buffers[index].size() < chunkSize);
			Queue(buffers[index], XAUDIO2_END_OF_STREAM * finalBuffer);
			m_sourceVoice->Start();

			index++;
			while (!m_shouldStop && !finalBuffer)
			{
				buffers[index] = wfr.ReadNextChunk(chunkSize);
				finalBuffer = (buffers[index].size() < chunkSize);
				
				if (m_shouldStop) break;
				Queue(buffers[index], XAUDIO2_END_OF_STREAM * finalBuffer);
				if (m_shouldStop || finalBuffer) break;

				m_callback->WaitForEnd();
				index = (index+1) % 2;
			}
			if (m_playingAsync)
			{
				HR hr = m_sourceVoice->Stop();
				hr.try_fail("Failed to stop Source Voice async");

				hr = m_sourceVoice->FlushSourceBuffers();
				hr.try_fail("Failed to flush source voice buffers");

				m_playingAsync = false;
			}
		}, std::move(fileReader));
}

void SourceVoice::Stop()
{
	m_shouldStop = true;

	if (!m_playingAsync)
	{
		HR hr = m_sourceVoice->Stop();
		hr.try_fail("Failed to stop Source Voice");

		hr = m_sourceVoice->FlushSourceBuffers();
		hr.try_fail("Failed to flush source voice queued buffers");
	}
}

bool SourceVoice::HasFinished()
{
	if (m_playingAsync)
	{
		return false;
	}

	XAUDIO2_VOICE_STATE state;
	m_sourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	if (state.BuffersQueued == 0)
	{
		HR hr = m_sourceVoice->Stop();
		hr.try_fail("Failed to stop source voice with no buffers queued");
	}

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

void SourceVoice::Queue(const std::vector<u8>& buffer, u32 bufferFlag)
{
	XAUDIO2_BUFFER xAudioBuffer = {
		.Flags = bufferFlag,
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
}
