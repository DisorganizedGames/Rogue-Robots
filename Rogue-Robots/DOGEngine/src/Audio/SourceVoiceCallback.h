#pragma once

namespace DOG
{
	class SourceVoiceCallback : public IXAudio2VoiceCallback
	{
	private:
		HANDLE streamEndEvent;
	public:
		SourceVoiceCallback()
		{
			streamEndEvent = CreateEventA(nullptr, false, false, nullptr);
		}
		~SourceVoiceCallback()
		{
			if (streamEndEvent)
				CloseHandle(streamEndEvent);
		};
	public:
		void OnStreamEnd() override
		{
			SetEvent(streamEndEvent);
		}

		void WaitForStreamEnd()
		{
			WaitForSingleObjectEx(streamEndEvent, INFINITE, true);
			ResetEvent(streamEndEvent);
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
