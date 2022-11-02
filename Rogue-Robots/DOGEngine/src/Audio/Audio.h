#pragma once
#include "AudioFileReader.h"
#include "../Core/AssetManager.h"
#include "../ECS/EntityManager.h"

namespace DOG
{
	class AudioDevice;

	class SourceVoice
	{
		friend class AudioDevice;
	public:

	private:
		WAVEFORMATEX m_wfx;
		IXAudio2SourceVoice* m_source = nullptr;
		std::atomic_bool m_stopped = true;
		std::atomic_bool m_async = false;

		u64 m_idx = 0;
		std::span<u8> m_externalBuffer;

		u64 m_ringIdx = 0;
		std::array<std::vector<u8>, 16> m_bufferRing;
		DOG::WAVFileReader m_asyncWFR;

		u64 m_lastSeek = 0;
		u64 m_lastSamplesPlayed = 0;
		u64 m_samplesPlayed = 0;

		std::mutex m_loopMutex;

		static constexpr u64 CHUNK_SIZE = 1024;

	public:
		SourceVoice() = delete;
		SourceVoice(IXAudio2* audioDevice, WAVEFORMATEX wfx);
		~SourceVoice() {
			std::scoped_lock<std::mutex> lock(m_loopMutex);
			if (m_source)
			{
				m_source->DestroyVoice();
				m_source = nullptr;
			}
		}

	public:
		void Play(std::span<u8> data);
		void PlayAsync();
		void Stop();
		void SetVolume(f32 volume);

		void SetOutputMatrix(const std::vector<f32>& matrix, IXAudio2Voice* dest);
		void SeekTo(f32 seconds);

		bool HasFinished();
		bool Stopped() { return m_stopped; }
		f32 SecondsPlayed();
		f32 AudioLengthInSeconds();

		void SetFileReader(DOG::WAVFileReader&& wfr) { this->m_asyncWFR = std::move(wfr); }
		void QueueNext();
		void QueueNextAsync();
	};



	class AudioDevice
	{
	private:
		IXAudio2* m_xaudio = nullptr;
		IXAudio2MasteringVoice* m_master = nullptr;
		XAUDIO2_VOICE_DETAILS m_masterDetails = {};

		X3DAUDIO_HANDLE m_x3daudio = {};

		std::array<std::unique_ptr<SourceVoice>, 64> m_sources = { nullptr };

		std::atomic_bool m_threadShouldDie = false;
		std::thread m_audioThread;

	public:
		AudioDevice();
		~AudioDevice();

		void HandleComponent(AudioComponent& comp, entity e);
		void Commit();

	private:
		void AudioThreadRoutine();
		u64 GetFreeVoice(const WAVEFORMATEX& m_wfx);
		void Handle3DComponent(SourceVoice* source, entity e);
	};

}