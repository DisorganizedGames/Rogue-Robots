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

void SourceVoice::PlayAsync(WAVFileReader&& fileReader)
{
	constexpr const u32 chunkSize = 4096;
	m_fileReader = std::move(fileReader);

	m_buffers.clear();
	m_buffers.resize(2);

	m_playingAsync = true;
	m_audioThread = std::thread([&] ()
		{
			u32 index = 1;
			bool endOfStream = false;

			m_buffers[0] = m_fileReader.ReadNextChunk(chunkSize);
			endOfStream = (m_buffers[0].size() < chunkSize);
			Queue(m_buffers[0], endOfStream * XAUDIO2_END_OF_STREAM);
			
			m_sourceVoice->Start();

			while (!endOfStream && m_playingAsync)
			{
				m_buffers[index] = m_fileReader.ReadNextChunk(chunkSize);
				endOfStream = (m_buffers[index].size() < chunkSize);
				Queue(m_buffers[index], endOfStream * XAUDIO2_END_OF_STREAM);

				m_callback->WaitForEnd();
				index = (index+1) % 2;
			}
			m_playingAsync = false;
			m_buffers.clear();
		});
}

void SourceVoice::Stop()
{
	if (m_playingAsync)
	{
		m_playingAsync = false;
		m_audioThread.join();
	}

	HR hr = m_sourceVoice->Stop();
	hr.try_fail("Failed to stop Source Voice");

	hr = m_sourceVoice->FlushSourceBuffers();
	hr.try_fail("Failed to flush source voice queued buffers");

	m_buffers.clear();
}

void SourceVoice::WaitForEnd()
{
	if (m_playingAsync)
	{
		m_audioThread.join();
		return;
	}
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
