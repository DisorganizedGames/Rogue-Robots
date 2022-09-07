#include "SourceVoice.h"

DOG::SourceVoice::SourceVoice(IXAudio2SourceVoice* sourceVoice, const WAVProperties& properties,
	std::unique_ptr<SourceVoiceCallback> callback, const SourceVoiceSettings& settings)

	: sourceVoice(sourceVoice), audioProperties(properties), callback(std::move(callback))
{
	voiceSettings = settings;
	sourceVoice->SetVolume(settings.volume);
}

DOG::SourceVoice::SourceVoice(SourceVoice&& other) noexcept
	: sourceVoice(other.sourceVoice), audioProperties(other.audioProperties), voiceSettings(other.voiceSettings)
{
	other.sourceVoice = nullptr;
}

DOG::SourceVoice::~SourceVoice()
{
	if (sourceVoice)
		sourceVoice->DestroyVoice();
}

void DOG::SourceVoice::Play(std::vector<u8>&& buffer)
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

	HR hr = sourceVoice->SubmitSourceBuffer(&xAudioBuffer);
	hr.try_fail("Failed to queue XAudio Buffer");

	hr = sourceVoice->Start();
	hr.try_fail("Failed to start playing queued XAudio Buffer");
}

void DOG::SourceVoice::WaitForEnd()
{
	XAUDIO2_VOICE_STATE state;
	sourceVoice->GetState(&state);
	if (state.BuffersQueued > 0)
	{
		callback->WaitForStreamEnd();
	}
	return;
}

bool DOG::SourceVoice::HasFinished()
{
	XAUDIO2_VOICE_STATE state;
	sourceVoice->GetState(&state);

	return state.BuffersQueued == 0;
}
