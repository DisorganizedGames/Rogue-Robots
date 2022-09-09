#pragma once
namespace DOG
{
	constexpr int m_nrOfPlayers = 4;
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
		};

		Server();
		~Server();

		void StartTcpServer();
	private:
		void ServerLoop(SOCKET listenSocket);
		void ClientThreadIdle();
		void ClientLoop(SOCKET clientSocket, int playerIndex);
		float TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency);

		float m_tickrate;

		ClientsData m_playersServer[m_nrOfPlayers];
		std::queue<std::function<void()>> m_clientThreadsQueue;
		std::mutex m_clientMutex;
		std::condition_variable m_clientMutexCondition;
		std::vector<std::thread>  m_clientThreads;
		std::vector<int>	m_playerIds;
	};
}