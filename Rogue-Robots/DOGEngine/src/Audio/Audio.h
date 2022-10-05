#pragma once
#define NOMINMAX
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
		std::vector<u8> m_externalBuffer;

		u64 m_ringIdx = 0;
		std::array<std::vector<u8>, 4> m_bufferRing;
		DOG::WAVFileReader m_asyncWFR;

		static constexpr u64 CHUNK_SIZE = 1024;

	public:
		SourceVoice() = delete;
		SourceVoice(IXAudio2* audioDevice, WAVEFORMATEX wfx);
		~SourceVoice() { if (m_source) m_source->DestroyVoice(); }

	public:
		void Play(std::vector<u8> data);
		void PlayAsync();
		void Stop();

		bool HasFinished();
		bool Stopped() { return m_stopped; }

		void SetFileReader(DOG::WAVFileReader&& wfr) { this->m_asyncWFR = std::move(wfr); }
		void QueueNext();
		void QueueNextAsync();
	};



	class AudioDevice
	{
	private:
		IXAudio2* m_xaudio = nullptr;
		IXAudio2MasteringVoice* m_master = nullptr;

		std::array<std::unique_ptr<SourceVoice>, 64> m_sources = { nullptr };

		std::atomic_bool m_threadShouldDie = false;
		std::thread m_audioThread;

	public:
		AudioDevice();
		~AudioDevice();

		void HandleComponent(AudioComponent& comp);
		void Commit();

	private:
		void AudioThreadRoutine();
		u64 GetFreeVoice(const WAVEFORMATEX& m_wfx);
	};

}