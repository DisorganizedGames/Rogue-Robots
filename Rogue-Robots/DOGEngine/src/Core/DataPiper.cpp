#include "DataPiper.h"

namespace DOG::piper
{
	static PipedData* s_pipedData{ nullptr };

	void SetPipe(PipedData* data)
	{	
		s_pipedData = data;
	}
	const PipedData* GetPipe()
	{
		assert(s_pipedData);
		return s_pipedData;
	}
}