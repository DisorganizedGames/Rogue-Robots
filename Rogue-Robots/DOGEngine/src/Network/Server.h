#pragma once
#include "Client.h"
namespace DOG
{
	class Server
	{
	public:
		Server();
		~Server();

		void StartTcpServer();
	private:
		void ServerReciveConnections(SOCKET listenSocket);
		void ServerPoll();
		void CloseSocket(int socketIndex);
		float TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency);

		float m_tickrate;

		Client::ClientsData m_playersServer[MAX_PLAYER_COUNT];
		std::vector<int>		m_playerIds;
		std::vector<int>		m_holdPlayerIds;
		std::vector<WSAPOLLFD>	m_clientsSockets;
		std::vector<WSAPOLLFD>	m_holdSockets;
	};
}