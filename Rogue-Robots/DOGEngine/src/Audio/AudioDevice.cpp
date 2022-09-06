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
