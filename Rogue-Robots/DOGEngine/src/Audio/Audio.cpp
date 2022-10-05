#include "Audio.h"

using namespace DOG;

AudioDevice::AudioDevice() : m_audioThread([](AudioDevice* self) { self->AudioThreadRoutine(); }, this)
{
	try
	{
		HR hr = XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
		hr.try_throw("Failed to initialize xaudio2 device");

		hr = m_xaudio->CreateMasteringVoice(&m_master);
		hr.try_throw("Failed to create xaudio2 mastering voice");
	}
	catch (HRError& e)
	{
		m_threadShouldDie = true;
		m_audioThread.detach();
		throw e;
	}
}

AudioDevice::~AudioDevice()
{
	m_threadShouldDie = true;
	m_audioThread.detach();

	for (auto& source : m_sources)
	{
		if (source)
			source.reset();
		source = nullptr;
	}

	m_master->DestroyVoice();
	m_xaudio->Release();
}

void AudioDevice::HandleComponent(AudioComponent& comp)
{
	if (!comp.playing && !comp.shouldPlay && !comp.shouldStop)
		return;

	if (comp.shouldPlay)
	{
		if (comp.playing)
		{
			m_sources[comp.source]->Stop();
			comp.playing = false;
		}

		AudioAsset* asset = AssetManager::Get().GetAsset<AudioAsset>(comp.assetID);
		auto prop = asset->properties;

		WAVEFORMATEX m_wfx = {
			.wFormatTag = (WORD)prop.format,
			.nChannels = prop.channels,
			.nSamplesPerSec = prop.sampleRate,
			.nAvgBytesPerSec = prop.channels * prop.sampleRate * prop.bps / 8,
			.nBlockAlign = static_cast<u16>(prop.channels * prop.bps / 8),
			.wBitsPerSample = prop.bps,
			.cbSize = 0,
		};

		auto freeVoice = (u32)GetFreeVoice(m_wfx);
		auto& source = m_sources[freeVoice];

		source = std::make_unique<SourceVoice>(m_xaudio, m_wfx);
		comp.source = freeVoice;

		if (asset->async)
		{
			source->SetFileReader(DOG::WAVFileReader(asset->filePath));
			source->PlayAsync();
		}
		else
		{
			source->Play(asset->audioData);
		}

		comp.shouldPlay = false;
		comp.playing = true;
	}

	if (comp.shouldStop)
	{
		m_sources[comp.source]->Stop();

		comp.playing = false;
		comp.shouldStop = false;
	}
}

void AudioDevice::Commit()
{
	m_xaudio->CommitChanges(XAUDIO2_COMMIT_NOW);
}

void AudioDevice::AudioThreadRoutine()
{
	while (!m_threadShouldDie)
	{
		for (auto& source: m_sources)
		{
			if (!source || source->Stopped())
				continue;

			if (source->m_async)
				source->QueueNextAsync();
			else
				source->QueueNext();
		}
	}
}

u64 AudioDevice::GetFreeVoice(const WAVEFORMATEX& m_wfx)
{
	// Find voice with matching WFX
	for (int i = 0; i < m_sources.size(); ++i)
	{
		auto& src = m_sources[i];
		if (!src || !src->HasFinished())
			continue;

		auto& srcWfx = src->m_wfx;
		if (srcWfx.wFormatTag == m_wfx.wFormatTag
			&& srcWfx.nChannels == m_wfx.nChannels
			&& srcWfx.nBlockAlign == m_wfx.nBlockAlign
			&& srcWfx.wBitsPerSample == m_wfx.wBitsPerSample)
		{
			src->m_source->SetSourceSampleRate(m_wfx.nSamplesPerSec);
			return i;
		}
	}

	// Find null-voice
	for (int i = 0; i < m_sources.size(); ++i)
	{
		auto& src = m_sources[i];
		if (!src)
			return i;
	}
	
	// Find unused voice
	for (int i = 0; i < m_sources.size(); ++i)
	{
		if (m_sources[i]->HasFinished())
		{
			m_sources[i].reset();
			return i;
		}
	}
	
	// Audio just couldn't be played, no matter how hard we tried
	// Solve this somehow? Or maybe just crash and we'll have to increase maximum voice-count
	assert(false && "failed to find empty voice");
	return u64(-1);
}

// ------- SOURCE VOICE ----------

SourceVoice::SourceVoice(IXAudio2* audioDevice, WAVEFORMATEX wfx) : m_wfx(std::move(wfx))
{
	HRESULT hr = audioDevice->CreateSourceVoice(&m_source, &m_wfx);
	if (FAILED(hr)) { throw false; }
}

void SourceVoice::Play(std::vector<u8> data)
{
	m_externalBuffer = data; // Not great
	m_idx = 0;

	QueueNext();

	m_source->SetVolume(2.0f);
	m_source->Start(0, 0);
	m_stopped = false;
}

void SourceVoice::PlayAsync()
{
	m_async = true;

	QueueNextAsync();

	m_source->SetVolume(2.0f);
	m_source->Start(0, 0);
	m_stopped = false;
}

void SourceVoice::Stop()
{
	m_stopped = true;
	m_source->SetVolume(0.0);
	m_source->FlushSourceBuffers();
	m_source->Discontinuity();
	m_externalBuffer.resize(0);
}

bool SourceVoice::HasFinished()
{
	XAUDIO2_VOICE_STATE state;
	m_source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	if (!m_stopped)
	{
		m_stopped = state.BuffersQueued == 0;
	}

	if (m_stopped)
	{
		m_source->Stop();
		for (auto& buf: m_bufferRing) buf.resize(0);
	}

	return state.BuffersQueued == 0;
}

void SourceVoice::QueueNext()
{
	XAUDIO2_VOICE_STATE state;
	m_source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	while (state.BuffersQueued < m_bufferRing.size())
	{
		auto it = m_externalBuffer.begin() + m_idx;

		auto& curBuffer = m_bufferRing[m_ringIdx++];

		u64 newSize = std::min(CHUNK_SIZE, m_externalBuffer.size()-m_idx);

		if (curBuffer.size() == 0)
		{
			curBuffer.resize(newSize);
		}

		std::copy(it, it + newSize, curBuffer.begin());

		XAUDIO2_BUFFER buf = {
			.AudioBytes = (u32)curBuffer.size(),
			.pAudioData = curBuffer.data(),
		};

		HR hr = m_source->SubmitSourceBuffer(&buf);
		hr.try_fail("Failed to submit source buffer");

		m_ringIdx %= m_bufferRing.size();

		m_idx += CHUNK_SIZE;

		if (m_idx >= m_externalBuffer.size())
		{
			m_idx = m_externalBuffer.size();
			Stop();
			break;
		}

		m_source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	}
}

void SourceVoice::QueueNextAsync()
{
	XAUDIO2_VOICE_STATE state;
	m_source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	while (state.BuffersQueued < m_bufferRing.size())
	{
		auto& curBuffer = m_bufferRing[m_ringIdx++];

		curBuffer = m_asyncWFR.ReadNextChunk(CHUNK_SIZE);

		XAUDIO2_BUFFER buf = {
			.AudioBytes = (u32)curBuffer.size(),
			.pAudioData = curBuffer.data(),
		};

		HR hr = m_source->SubmitSourceBuffer(&buf);
		hr.try_fail("Failed to submit source buffer");

		m_ringIdx %= m_bufferRing.size();

		m_idx += CHUNK_SIZE;

		if (m_idx > m_externalBuffer.size())
		{
			m_idx = m_externalBuffer.size();
			m_source->Discontinuity();
			break;
		}

		m_source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	}
}
