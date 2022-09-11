#pragma once
#include "Server.h"
namespace DOG
{
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
			char inputs[32];
			int input_lentgh;
		};
		Client();
		~Client();
		int ConnectTcpServer(std::string ipAdress);
		ClientsData* SendandReciveTcp(ClientsData input);
		ClientsData* GetClientsData();
	private:
		ClientsData m_playersClient[m_nrOfPlayers];
		SOCKET m_connectSocket;
		
	};
}