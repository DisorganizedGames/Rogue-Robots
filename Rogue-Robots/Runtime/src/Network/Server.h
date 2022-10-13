#pragma once
#include <DOGEngine.h>
#include "Client.h"
#include "..\Game\GameComponent.h"


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

	private:
		void GameLoopUdp();
		void ReciveLoopUdp();
		int m_upid;
		int m_reciveupid;
		std::atomic_bool m_gameAlive;

		std::thread m_reciveConnectionsTcp;
		std::thread m_loopTcp;
		std::thread m_loopUdp;
		std::thread m_reciveLoopUdp;
		Client::ClientsData m_playersServer[MAX_PLAYER_COUNT];
		Client::UdpData m_outputUdp;
		std::vector<UINT8>		m_playerIds;
		std::vector<UINT8>		m_holdPlayerIds;
		std::vector<WSAPOLLFD>	m_clientsSocketsTcp;
		std::vector<WSAPOLLFD>	m_holdSocketsTcp;
		float m_tickrateTcp;
		float m_tickrateUdp;
		std::mutex m_mut;
		Client::PlayerNetworkComponent m_holdPlayersUdp[MAX_PLAYER_COUNT];


	};
