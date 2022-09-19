#pragma once

namespace DOG
{
	class SourceVoiceCallback : public IXAudio2VoiceCallback
	{
	private:
		HANDLE m_streamEndEvent;
		HANDLE m_bufferEndEvent;
		bool m_hardEnd = false;
	public:
		SourceVoiceCallback()
		{
			m_streamEndEvent = CreateEventA(nullptr, false, false, nullptr);
			m_bufferEndEvent = CreateEventA(nullptr, false, false, nullptr);
		}
		~SourceVoiceCallback()
		{
			if (m_streamEndEvent)
				CloseHandle(m_streamEndEvent);
		};

	public:

		void TriggerEnd()
		{
			SetEvent(m_streamEndEvent);
			m_hardEnd = true;
		}

		void OnStreamEnd() override
		{
			SetEvent(m_streamEndEvent);
		}

		void OnBufferEnd(void*) override
		{
			SetEvent(m_bufferEndEvent);
		}

		void WaitForStreamEnd()
		{
			if (!m_hardEnd)
			{
				WaitForSingleObjectEx(m_streamEndEvent, INFINITE, true);
			}
		}

		void WaitForEnd()
		{
			if (!m_hardEnd)
			{
				std::vector<HANDLE> events = { m_streamEndEvent, m_bufferEndEvent };
				WaitForMultipleObjectsEx(2, &events[0], false, INFINITE, true);
			}
		}

		// Unimplemented callbacks
		void OnBufferStart(void*) override {}
		void OnLoopEnd(void*) override {}
		void OnVoiceError(void*, HRESULT) override {}
		void OnVoiceProcessingPassEnd() override {}
		void OnVoiceProcessingPassStart(u32) override {}
	};
}

