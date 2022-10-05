#include "AudioManager.h"
#include "../ECS/EntityManager.h"

using namespace DOG;

void AudioManager::Initialize()
{
	try
	{
		s_device = std::make_unique<AudioDevice>();
		s_deviceInitialized = true;
	}
	catch (DOG::HRError& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void AudioManager::Destroy()
{
	s_device.reset();
}

void AudioManager::AudioSystem()
{
	if (!s_deviceInitialized)
		return;

	EntityManager::Get().Collect<AudioComponent>().Do([](AudioComponent& ac)
		{
			s_device->HandleComponent(ac);
		});
	
	s_device->Commit();
}
