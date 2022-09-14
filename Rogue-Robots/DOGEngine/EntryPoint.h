#pragma once
#include "src/Core/Application.h"
	int main(int, char**)
	{

		HR hr = CoInitialize(nullptr);
		hr.try_fail("Failed to initialize COM");

		std::unique_ptr<DOG::Application> app{ nullptr };
		while (DOG::ApplicationManager::ShouldRestartApplication())
		{
			if (!DOG::ApplicationManager::ShouldRestartApplication())
				app = std::move(CreateApplication());
			else
				app->OnRestart();

			app->Run();
		}

		CoUninitialize();
		
		return 0;
	}

	extern std::unique_ptr<DOG::Application> CreateApplication() noexcept;
