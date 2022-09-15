#pragma once

struct AudioPlayerComponent
{
	u64 audioID;
	f32 volume = 2.0f;
	// ...
	i64 voiceID = -1;
};