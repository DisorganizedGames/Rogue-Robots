#pragma once
#include "Client.h"

namespace DOG
{
	class Server
	{
	public:
		Server();
		~Server();

		bool StartTcpServer();
		std::string GetIpAddress();
	private:
		void ServerReciveConnectionsTCP(SOCKET listenSocket);
		void ServerPollTCP();
		void CloseSocketTCP(int socketIndex);
		float TickTimeLeftTCP(LARGE_INTEGER t, LARGE_INTEGER frequency);
		std::atomic_bool m_serverAlive;
		float m_tickrate;
		std::thread m_reciveConnections;
		std::thread m_serverLoop;
		Client::ClientsData m_playersServer[MAX_PLAYER_COUNT];
		std::vector<int>		m_playerIds;
		std::vector<int>		m_holdPlayerIds;
		std::vector<WSAPOLLFD>	m_clientsSockets;
		std::vector<WSAPOLLFD>	m_holdSockets;

	};
}