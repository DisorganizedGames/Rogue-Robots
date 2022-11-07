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

		m_master->GetVoiceDetails(&m_masterDetails);

		hr = X3DAudioInitialize(SPEAKER_STEREO, 300.f, m_x3daudio);
		hr.try_fail("Failed to initialize x3daudio");
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

void AudioDevice::HandleComponent(AudioComponent& comp, entity e)
{
	if (comp.shouldPlay)
	{
		if (comp.playing)
		{
			m_sources[comp.source]->Stop();
			comp.playing = false;
		}

		AudioAsset* asset = AssetManager::Get().GetAsset<AudioAsset>(comp.assetID);
		auto wfx = asset->properties;

		auto freeVoice = (u32)GetFreeVoice(wfx);
		auto& source = m_sources[freeVoice];

		source = std::make_unique<SourceVoice>(m_xaudio, wfx);
		comp.source = freeVoice;

		if (asset->async)
		{
			source->SetFileReader(WAVFileReader(asset->filePath));
			source->PlayAsync();
		}
		else
		{
			source->Play(asset->audioData);
		}

		comp.shouldPlay = false;
		comp.playing = true;
	}

	// Return early if the component has no voice assigned
	if (comp.source == u32(-1))
	{
		return;
	}

	auto& source = m_sources[comp.source];
	if (source->Stopped())
	{
		comp.playing = false;
		comp.source = u32(-1);
	}

	if (comp.shouldStop)
	{
		source->Stop();

		comp.playing = false;
		comp.shouldStop = false;
	}

	if (!comp.playing)
		return;

	if (comp.loop)
	{
		if (comp.loopEnd <= comp.loopStart)
		{
			comp.loopEnd = source->AudioLengthInSeconds() - 0.1f;
		}

		f32 played = source->SecondsPlayed();
		
		constexpr f32 timeToFade = 0.05f;

		f32 startFadeOut = comp.loopEnd - timeToFade;

		if (played >= startFadeOut)
		{
			f32 fadeOutTime = played - startFadeOut;
			source->m_source->SetVolume(2.f * (1 - fadeOutTime / timeToFade));
		}

		if (played >= comp.loopEnd)
		{
			std::scoped_lock<std::mutex> lock(source->m_loopMutex);
			source->m_source->FlushSourceBuffers();
			source->SeekTo(comp.loopStart);
			source->PlayAsync();
		}
	}

	if (comp.is3D)
	{
		Handle3DComponent(source.get(), e);
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

			std::scoped_lock<std::mutex> lock(source->m_loopMutex);

			if (!source || !source->m_source) 
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
		std::cout << "Finding unused voice\n";
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

void AudioDevice::Handle3DComponent(SourceVoice* source, entity e)
{
	std::vector<f32> azimuths(source->m_wfx.nChannels);
	std::iota(azimuths.begin(), azimuths.end(), 0.f);
	std::transform(azimuths.begin(), azimuths.end(), azimuths.begin(), [](auto angle) { return (angle + 0.5f) * X3DAUDIO_PI; });

	auto playPos = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();

	X3DAUDIO_EMITTER es = {
		.OrientFront = {0, 0, 1},
		.OrientTop = {0, 1, 0},
		.Position = playPos,
		.ChannelCount = source->m_wfx.nChannels,
		.ChannelRadius = 1.0f,
		.pChannelAzimuths = azimuths.data(),
		.CurveDistanceScaler = 1.0f,
	};

	X3DAUDIO_DSP_SETTINGS dspSettings = {
		.SrcChannelCount = es.ChannelCount,
		.DstChannelCount = m_masterDetails.InputChannels,
	};
	bool listenersExist = false;
	EntityManager::Get().Collect<AudioListenerComponent, TransformComponent>()
		.Do([&](AudioListenerComponent& /*listener*/, TransformComponent& transform)
			{
				listenersExist = true;
				std::vector<f32> matrix(dspSettings.DstChannelCount * dspSettings.SrcChannelCount);
				X3DAUDIO_LISTENER ls = {
					.OrientFront = transform.worldMatrix.Forward(),
					.OrientTop = transform.worldMatrix.Up(),
					.Position = transform.GetPosition(),
					.Velocity = {}, // For doppler effect
					.pCone = nullptr, // Cones not supported yet
				};

				dspSettings.pMatrixCoefficients = matrix.data();
				X3DAudioCalculate(m_x3daudio, &ls, &es, X3DAUDIO_CALCULATE_MATRIX, &dspSettings);
				source->SetOutputMatrix(matrix, m_master);
			});
	if (!listenersExist)
	{
		source->SetVolume(0.f);
	}
}

// ------- SOURCE VOICE ----------

SourceVoice::SourceVoice(IXAudio2* audioDevice, WAVEFORMATEX wfx) : m_wfx(std::move(wfx))
{
	HRESULT hr = audioDevice->CreateSourceVoice(&m_source, &m_wfx);
	if (FAILED(hr)) { throw false; }
}

void SourceVoice::Play(std::span<u8> data)
{
	m_externalBuffer = data;
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
	m_async = false;
	m_source->SetVolume(0.0);
	m_source->FlushSourceBuffers();
	m_source->Discontinuity();
	m_samplesPlayed = 0;
	m_lastSamplesPlayed = 0;
}

void SourceVoice::SetVolume(f32 volume)
{
	m_source->SetVolume(volume);
}

void SourceVoice::SetOutputMatrix(const std::vector<f32>& matrix, IXAudio2Voice* dest)
{
	auto destChannels = matrix.size()/m_wfx.nChannels;
	HR hr = m_source->SetOutputMatrix(dest, m_wfx.nChannels, (u32)destChannels, matrix.data());
	hr.try_fail("Failed to set output matrix for source voice");
}

void SourceVoice::SeekTo(f32 seconds)
{
	u32 loopStartSample = static_cast<u32>(seconds * m_wfx.nSamplesPerSec);
	m_asyncWFR.SeekToSample(loopStartSample);

	m_samplesPlayed = loopStartSample;
	m_lastSeek = m_samplesPlayed;

	XAUDIO2_VOICE_STATE state;
	m_source->GetState(&state);
	m_lastSamplesPlayed = state.SamplesPlayed;
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
		Stop();
		for (auto& buf: m_bufferRing) buf.resize(0);
	}

	return state.BuffersQueued == 0;
}

f32 SourceVoice::SecondsPlayed()
{
	XAUDIO2_VOICE_STATE state;
	m_source->GetState(&state);

	m_samplesPlayed = m_lastSeek + (state.SamplesPlayed - m_lastSamplesPlayed);

	return static_cast<f32>(m_samplesPlayed) / static_cast<f32>(m_wfx.nSamplesPerSec);
}

f32 SourceVoice::AudioLengthInSeconds()
{
	std::scoped_lock lock(m_loopMutex);

	if (m_async)
	{
		auto bytes = m_asyncWFR.DataSize();
		auto seconds = bytes / static_cast<f32>(m_wfx.nAvgBytesPerSec);
		return seconds;
	}
	return 0.f;
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
	const auto bufferRingSize = m_bufferRing.size();

	while (state.BuffersQueued < bufferRingSize)
	{
		auto& curBuffer = m_bufferRing[m_ringIdx++];

		curBuffer = m_asyncWFR.ReadDataChunk(CHUNK_SIZE);
		if (curBuffer.size() < CHUNK_SIZE)
		{
			Stop();
			break;
		}

		XAUDIO2_BUFFER buf = {
			.AudioBytes = (u32)curBuffer.size(),
			.pAudioData = curBuffer.data(),
		};

		HR hr = m_source->SubmitSourceBuffer(&buf);
		hr.try_fail("Failed to submit source buffer");

		m_ringIdx %= m_bufferRing.size();

		m_source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	}
}
