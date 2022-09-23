#pragma once

struct AudioPlayerComponent
{
	u32 audioID;
	f32 volume = 2.0f;
	// ...
	i64 voiceID = -1;
};