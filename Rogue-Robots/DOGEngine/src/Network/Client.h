#pragma once
#include "Server.h"
namespace DOG
{
	constexpr int MAX_PLAYER_COUNT = 4;

	class Client
	{
	public:
		struct ClientsData
		{
			int player_nr = 0;
			bool w = false;
			bool a = false;
			bool s = false;
			bool d = false;
			char inputs[64] = {0};
			int inputLength = 0;
		};
		Client();
		~Client();
		int ConnectTcpServer(std::string ipAdress);
		ClientsData* SendandReciveTcp(ClientsData input);
		ClientsData AddString(ClientsData player, std::string inputs);
		ClientsData CleanClientsData(ClientsData player);
	private:
		ClientsData m_playersClient[m_maxNrOfPlayer];
		SOCKET m_connectSocket;
		
	};
}