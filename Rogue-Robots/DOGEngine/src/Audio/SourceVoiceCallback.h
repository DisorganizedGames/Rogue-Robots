#pragma once

namespace DOG
{
	class SourceVoiceCallback : public IXAudio2VoiceCallback
	{
	private:
		HANDLE m_streamEndEvent;
	public:
		SourceVoiceCallback()
		{
			m_streamEndEvent = CreateEventA(nullptr, false, false, nullptr);
		}
		~SourceVoiceCallback()
		{
			if (m_streamEndEvent)
				CloseHandle(m_streamEndEvent);
		};
	public:
		void OnStreamEnd() override
		{
			SetEvent(m_streamEndEvent);
		}

		void WaitForStreamEnd()
		{
			WaitForSingleObjectEx(m_streamEndEvent, INFINITE, true);
			ResetEvent(m_streamEndEvent);
		}

		// Unimplemented callbacks
		void OnBufferEnd(void*) override {}
		void OnBufferStart(void*) override {}
		void OnLoopEnd(void*) override {}
		void OnVoiceError(void*, HRESULT) override {}
		void OnVoiceProcessingPassEnd() override {}
		void OnVoiceProcessingPassStart(u32) override {}
	};
}

