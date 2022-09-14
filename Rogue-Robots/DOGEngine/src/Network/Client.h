#pragma once

namespace DOG
{
	constexpr int MAX_PLAYER_COUNT = 4;

	class Client
	{
	public:
		struct ClientsData
		{
			int player_nr = 0;
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
		ClientsData m_playersClient[MAX_PLAYER_COUNT];
		SOCKET m_connectSocket;
		
	};
}