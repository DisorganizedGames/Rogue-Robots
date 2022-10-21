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

	EntityManager::Get().Collect<AudioComponent>().Do([](entity e, AudioComponent& ac)
		{
			s_device->HandleComponent(ac, e);
		});
	
	s_device->Commit();
}

void AudioManager::StopAudioOnDeferredEntities()
{
	if (!s_deviceInitialized)
		return;

	EntityManager::Get().Collect<AudioComponent, DeferredDeletionComponent>().Do([](AudioComponent& ac, DeferredDeletionComponent&)
		{
			ac.shouldStop = true;
		});

	AudioSystem(); // Stop entities that have shouldStop applied

	s_device->Commit();
}
