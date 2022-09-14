//#pragma once
//#include "src/Core/Application.h"
//	int main(int, char**)
//	{
//
//		HR hr = CoInitialize(nullptr);
//		hr.try_fail("Failed to initialize COM");
//
//		std::unique_ptr<DOG::Application> app{ nullptr };
//		while (DOG::ApplicationManager::ShouldRestartApplication())
//		{
//			if (!DOG::ApplicationManager::ShouldRestartApplication())
//				app = std::move(CreateApplication());
//			else
//				app->OnRestart();
//
//			app->Run();
//		}
//
//		CoUninitialize();
//		
//		return 0;
//	}
//
//	extern std::unique_ptr<DOG::Application> CreateApplication() noexcept;
#pragma once
#include "src/Core/Application.h"
#include "src/Network/Server.h"
#include "src/Network/Client.h"


int main(int, char**)
{
	DOG::Server serverTest;
	DOG::Client clientTest;
	DOG::Client::ClientsData testInput;
	DOG::Client::ClientsData* testOutput;
	if(getchar() == 's')
		serverTest.StartTcpServer();
	testInput.player_nr = clientTest.ConnectTcpServer("192.168.1.55");

	//HR hr = CoInitialize(nullptr);
	//hr.try_fail("Failed to initialize COM");

	//std::unique_ptr<DOG::Application> app{ nullptr };
	//while (DOG::ApplicationManager::ShouldRestartApplication())
	//{
	//	if (!DOG::ApplicationManager::ShouldRestartApplication())
	//		app = std::move(CreateApplication());
	//	else
	//		app->OnRestart();

	//	app->Run();
	//}

	//CoUninitialize();
	while (true) {
		std::string inputs = "";
		if (GetAsyncKeyState(87))
		{
			inputs.push_back('w');

		}
		if (GetAsyncKeyState(68))
		{
			inputs.push_back('d');

		}
		if (GetAsyncKeyState(83))
		{
			inputs.push_back('s');

		}
		if (GetAsyncKeyState(65))
		{
			inputs.push_back('a');

		}
		testInput = clientTest.AddString(testInput, inputs);
		testOutput = clientTest.SendandReciveTcp(testInput);
		for (int i = 0; i < 4; i++)
		{
			for(int j = 0; j < testOutput[i].inputLength; j++)
				std::cout << "From player " << testOutput[i].player_nr << ": " <<
					testOutput[i].inputs[0] << testOutput[i].inputs[1] << std::endl;
		}

		testInput = clientTest.CleanClientsData(testInput);
		continue;
	}
	return 0;
}

extern std::unique_ptr<DOG::Application> CreateApplication() noexcept;