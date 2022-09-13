#pragma once
#include "Client.h"
namespace DOG
{
	class Server
	{
	public:
		struct ClientsData
		{
			int player_nr = 0;
			bool w = false;
			bool a = false;
			bool s = false;
			bool d = false;
			char inputs[64] = { 0 };
			int inputLength = 0;
		};
		Server();
		~Server();

		void StartTcpServer();
	private:
		void ServerReciveConnections(SOCKET listenSocket);
		void ServerPoll();
		void CloseSocket(int socketIndex);
		float TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency);

		float m_tickrate;

		ClientsData m_playersServer[MAX_PLAYER_COUNT];
		std::vector<int>		m_playerIds;
		std::vector<int>		m_holdPlayerIds;
		std::vector<WSAPOLLFD>	m_clientsSockets;
		std::vector<WSAPOLLFD>	m_holdSockets;
	};
}