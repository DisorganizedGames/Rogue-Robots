#pragma once
#include <DOGEngine.h>
#include "Client.h"
#include "..\Game\GameComponent.h"
#include "Network.h"

	class Server
	{
	public:
		Server();
		~Server();

		bool StartTcpServer();
		std::string GetIpAddress();
		INT8 GetNrOfConnectedPlayers();
		void SetMulticastAdress(const char* adress);
		static float TickTimeLeftTCP(LARGE_INTEGER t, LARGE_INTEGER frequency);
		void StopReceiving();
		void SetLevelIndex(u16 levelIndex);
	private:
		void ServerReciveConnectionsTCP(SOCKET listenSocket);
		void ServerPollTCP();
		void CloseSocketTCP(int socketIndex);

	private:
		void GameLoopUdp();
		void ReciveLoopUdp();
		void ReadInGeneratedLevel();
		int m_upid;
		int m_reciveupid;
		std::atomic_bool m_gameAlive;

		std::thread m_reciveConnectionsTcp;
		std::thread m_loopTcp;
		std::thread m_loopUdp;
		std::thread m_reciveLoopUdp;
		UdpData m_outputUdp;
		std::vector<u8>		m_playerIds;
		std::vector<u8>		m_holdPlayerIds;
		std::vector<WSAPOLLFD>	m_clientsSocketsTcp;
		std::vector<WSAPOLLFD>	m_holdSocketsTcp;
		std::mutex m_mut;
		PlayerNetworkComponentUdp m_holdPlayersUdp[MAX_PLAYER_COUNT];
		char m_multicastAdress[16];
		bool m_lobbyStatus;
		bool m_reciveConnections;
		LobbyData m_lobbyData;
		char m_level[204800];
		u8 m_enablePlay;
	};
