#pragma once
namespace DOG
{
	constexpr int m_nrOfPlayers = 1;
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
			char inputs[256];
			int input_lentgh;
		};
		Server();
		~Server();

		void StartTcpServer();
	private:
		void ServerReciveConnections(SOCKET listenSocket);
		void ClientThreadIdle();
		void Lobby(SOCKET clientSocket, int playerIndex);
		void ClientLoop(SOCKET clientSocket, int playerIndex);
		void ServerSend();
		void ServerPoll();
		void ServerRecive(SOCKET clientSocket, int playerIndex);
		float TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency);

		float m_tickrate;

		fd_set m_connectedSockets;
		fd_set m_holdSockets;
		WSAPOLLFD m_clientPoll;
		ClientsData m_playersServer[m_nrOfPlayers];
		std::queue<std::function<void()>> m_clientThreadsQueue;
		std::mutex m_clientMutex;
		std::condition_variable m_clientMutexCondition;
		std::vector<std::thread>  m_clientThreads;
		std::vector<int>	m_playerIds;
		std::vector<WSAPOLLFD> m_clientsSockets;
	};
}