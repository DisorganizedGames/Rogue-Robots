#include "AudioDevice.h"
using namespace DOG;

AudioDevice::AudioDevice()
{
	HR hr = XAudio2Create(&xAudio);
	hr.try_fail("Failed to create XAudio2 Device");

#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfig = {
		.TraceMask = XAUDIO2_LOG_WARNINGS,
		.BreakMask = XAUDIO2_LOG_WARNINGS,
		.LogThreadID = true,
		.LogFileline = true,
		.LogFunctionName = true,
		.LogTiming = false,
	};

	xAudio->SetDebugConfiguration(&debugConfig);
#endif

	hr = xAudio->CreateMasteringVoice(&masteringVoice);
	hr.try_fail("Failed to create XAudio2 Mastering Voice");
}

DOG::AudioDevice::~AudioDevice()
{
	masteringVoice->DestroyVoice();
	if (xAudio)
	{
		xAudio->Release();
	}
}

SourceVoice DOG::AudioDevice::CreateSourceVoice(const WAVProperties& options, const SourceVoiceSettings& settings)
{
	assert(xAudio);

	WAVEFORMATEX wfx = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = options.channels,
		.nSamplesPerSec = options.sampleRate,
		.nAvgBytesPerSec = options.channels * options.sampleRate * options.bps / 8,
		.nBlockAlign = static_cast<u16>(options.channels * options.bps / 8),
		.wBitsPerSample = options.bps,
		.cbSize = 0,
	};

	auto callback = std::make_unique<SourceVoiceCallback>();

	IXAudio2SourceVoice* source;
	HR hr = xAudio->CreateSourceVoice(&source, &wfx, 0, 2.f, callback.get());
	hr.try_fail("Failed to create Source Voice");

	return SourceVoice(source, options, std::move(callback), settings);
}

